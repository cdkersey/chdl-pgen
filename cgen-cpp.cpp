#include <iostream>

#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "type.h"
#include "if.h"
#include "cgen-cpp.h"

using namespace bscotch;

static int which_suc(if_bb *a, if_bb *b) {
  for (unsigned i = 0; i < a->suc.size(); ++i)
    if (a->suc[i] == b) return i;
  return -1;
}

static std::string arg_name(if_bb *b, if_val *v) {
  std::ostringstream out;
  
  bool in_live_in = false;
  for (auto &x : b->live_in)
    if (x == v) in_live_in = true;

  if (in_live_in) {
    out << "s.bb" << b->id << "_in.val" << v->id;
  } else {
    out << "val" << v->id;
  }

  return out.str();
}

static const char *op_string(if_op op, bool bit) {
  switch(op) {
  case VAL_NEG: return "-";
  case VAL_NOT: return bit ? "!" : "~";
  case VAL_ADD: return "+";
  case VAL_SUB: return "-";
  case VAL_MUL: return "*";
  case VAL_DIV: return "/";
  case VAL_AND: return bit ? "&&" : "&";
  case VAL_OR: return bit ? "||" : "|";
  case VAL_XOR: return bit ? "!=" : "^";
  case VAL_EQ: return "==";
  case VAL_LT: return "<";
  default: return "UNKNOWN_OP";
  }
}

static if_val *find_call_or_spawn(if_bb &b) {
  for (auto &v : b.vals)
    if (v->op == VAL_CALL || v->op == VAL_SPAWN)
      return v;

  return NULL;
}

static if_val *find_ret(if_bb &b) {
 for (auto &v : b.vals)
   if (v->op == VAL_RET)
      return v;

  return NULL;
}

static int insert_type(
  std::map<bscotch::type, int> &m, int &count, const bscotch::type &t
)
{
  if (!m.count(t)) {
    m[t] = count++;

    // Recursively add component types for structs and arrays.
    if (is_struct(t))
      for (unsigned i = 0; i < t.get_n_fields(); ++i)
        insert_type(m, count, t.get_field_type(i));

    if (is_static_array(t) || is_sram_array(t))
      insert_type(m, count, element_type(t));
  }

  return m[t];
}

static void catalog_types(std::map<bscotch::type, int> &m, bscotch::if_prog &p)
{
  int count = 0;

  for (auto &s : p.global_vars) {
    insert_type(m, count, s.second.t);
  }
  
  for (auto &f : p.functions) {
    insert_type(m, count, f.second.rtype);
    for (auto &b : f.second.bbs) {
      for (auto &v : b->vals) {
        insert_type(m, count, v->t);
        for (auto &a : v->args)
          insert_type(m, count, a->t);
      }
    }
  }
}

static void show_catalog(std::map<bscotch::type, int> &m) {
  using namespace std;
  for (auto &t : m) {
    cout << "Type " << t.second << ": ";
    bscotch::type u = t.first;
    print(cout, u);
    cout << endl;
  }
}

static std::string type_cpp(const bscotch::type &t,
                            std::map<bscotch::type, int> &m)
{
  using namespace std;

  ostringstream oss;

  if (is_bit_type(t)) {
    oss << "bool";
  } else if (is_integer_type(t)) {
    if (t.type_vec[0] == TYPE_S) oss << "si<" << t.type_vec[1] << '>';
    else oss << "ui<" << t.type_vec[1] << '>';
  } else if (is_static_array(t) || is_sram_array(t)) {
    oss << "array<" << array_len(t) << ", " << type_cpp(element_type(t), m)
        << '>';
  } else if (is_struct(t)) {
    if (m.count(t)) {
      oss << "struct" << m[t];
    } else {
      oss << "struct {";
      for (unsigned i = 0; i < t.get_n_fields(); ++i) {
        oss << type_cpp(t.get_field_type(i), m) << ' '
            << t.get_field_name(i) << endl;
      }
      oss << '}';
    }
  }

  return oss.str();
}

static void gen_printer(std::ostream &out, int id, const bscotch::type &t) {
  using namespace std;

  out << "std::ostream &operator<<(std::ostream &out, const struct"
      << id << " &x) {" << endl;

  out << "  out << '{';" << endl;
  for (unsigned i = 0; i < t.get_n_fields(); ++i) {
    std::string field = t.get_field_name(i);
    out << "  out ";
    if (i != 0) out << "<< \", \"";
    out << "<< \"" << field << " = \" << x." << field << ';' << endl;
  }
  out << "  out << '}';" << endl
      << "  return out;" << endl
      << '}' << endl << endl;
}

static void assignment_body(std::ostream &out, const bscotch::type &t,
                            std::map<bscotch::type, int> &m)
{
  using namespace std;
  
  int size = 0;
  for (unsigned i = 0; i < t.get_n_fields(); ++i) {
    out << "    " << t.get_field_name(i) << " = x >> " << size << ';' << endl;
    size += t.get_field_type(i).size();
  }
  out << "    return *this;" << endl;
}

static void gen_struct_decls(std::ostream &out, std::map<bscotch::type, int> &m)
{
  using namespace std;

  // TODO: keep track of dependencies to guarantee structs containing struct
  // fields will be declared in the proper order.
  
  for (auto &t : m) {
    if (is_struct(t.first)) {
      out << "struct struct" << t.second << '{' << endl;

      for (unsigned i = 0; i < t.first.get_n_fields(); ++i) {
        out << "  " << type_cpp(t.first.get_field_type(i), m) << ' '
            << t.first.get_field_name(i) << ';' << endl;
      }

      // Generate assignment operators
      out << "  struct" << t.second << " &operator=(int x) {" << endl;
      assignment_body(out, t.first, m);
      out << "  }" << endl;

      out << "  struct" << t.second << " &operator=(ui<"
          << t.first.size() << "> x) {" << endl;
      assignment_body(out, t.first, m);
      out << "  }" << endl;

      out << "};" << endl << endl;
      
      gen_printer(out, t.second, t.first);
    }
  }
}

template <typename T> static bool is_subset(std::set<T> &a, std::set<T> &b) {
  for (auto &x : a) if (!b.count(x)) return false;
  return true;
}

// Reorder blocks so broadcast variables are
static void order_blocks(std::vector<if_bb*> &out,
                         const std::vector<if_bb*> &in)
{
  using namespace std;

  map<if_bb*, set<if_bb*> > bcast_st_ld, bcast_ld_st;

  set<if_staticvar*> vars;
  map<if_staticvar*, set<if_bb*> > loads;
  map<if_staticvar*, if_bb*> stores;

  // Find constraints.  
  for (auto &b : in) {
    for (auto &v : b->vals) {
      if (v->op == VAL_LD_STATIC && v->static_arg->broadcast) {
        loads[v->static_arg].insert(b);
        vars.insert(v->static_arg);
      } else if (v->op == VAL_ST_STATIC && v->static_arg->broadcast) {
        stores[v->static_arg] = b;
        vars.insert(v->static_arg);
      }
    }
  }

  for (auto &s : vars)
    bcast_st_ld[stores[s]] = loads[s];

  // Reverse constraints.
  for (auto &x : bcast_st_ld)
    for (auto &y : x.second)
      bcast_ld_st[y].insert(x.first);
      
  // Scan through repeatedly until no constraints are left.
  set<if_bb*> emitted;
  bool changed;
  do {
    changed = false;
    for (auto &b : in) {
      if (emitted.count(b)) continue;

      if (bcast_ld_st.count(b))
        if (is_subset(bcast_ld_st[b], emitted))
          bcast_ld_st.erase(b);

      if (!bcast_ld_st.count(b)) {
        out.push_back(b);
        emitted.insert(b);
        changed = true;
      }
    }
  } while (bcast_ld_st.size() && changed);

  if (!changed) {
    cerr << "Cycle in broadcast var dependencies." << endl;
    abort();
  }
}

static void gen_val(std::ostream &out, std::string fname, if_bb &b, if_val &v, std::map<bscotch::type, int> &m) {
  using namespace std;

  ostringstream val_name;
  val_name << "val" << v.id;
  if (!is_void_type(v.t) && v.op != VAL_PHI && v.op != VAL_SPAWN) {
    out << "    " << type_cpp(v.t, m) << ' ' << val_name.str() << ';' << endl;
  }

  if (v.pred)
    out << "    if (" << arg_name(&b, v.pred) << ")" << endl << "  ";
  
  if (v.op == VAL_PHI) {
    // Handled in arbiter.
  } else if (v.op == VAL_NEG || v.op == VAL_NOT) {
    out << "    val" << v.id << " = " << op_string(v.op, is_bit_type(v.t))
        << arg_name(&b, v.args[0]) << ';' << endl;
  } else if (v.op == VAL_OR_REDUCE || v.op == VAL_AND_REDUCE) {
    out << "    val" << v.id << " = ";
    if (v.op == VAL_OR_REDUCE) out << "or_reduce";
    else out << "and_reduce";
    out << '(' << arg_name(&b, v.args[0]) << ");" << endl;
  } else if (v.op == VAL_CONST) {
    out << "    val" << v.id << " = 0x" << to_hex(v.const_val) << "ull;"
        << endl;
  } else if (v.op == VAL_CONCATENATE) {
    if (v.args.size() == 1) {
      out << "    val" << v.id << " = " << arg_name(&b, v.args[0]) << ';'
          << endl;
    } else {
      out << "    cat(val" << v.id << ')';
      for (unsigned i = 0; i < v.args.size(); ++i)
        out << '(' << arg_name(&b, v.args[i]) << ')';
      out << ';' << endl;
    }
  } else if (v.op == VAL_ARG) {
    out << "    val" << v.id << " = s.bb" << b.id << "_in.arg"
        << v.static_access_id << ';' << endl;
  } else if (v.op == VAL_RET) {
    if (v.args.size() && !is_void_type(v.args[0]->t))
      out << "    s.ret.rval = " << arg_name(&b, v.args[0]) << ';';
    out << "    s.ret.valid = true;" << endl
        << "    s.ret.live = s.bb" << b.id << "_in.live;" << endl
        << "    s.bb" << b.id << "_out.valid = false;" << endl;
  } else if (v.op == VAL_LD_STATIC) {
    out << "    val" << v.id << " = s.static_var_" << v.static_arg->name << ';'
        << endl;
  } else if (v.op == VAL_LD_BCAST_VALID) {
    out << "    val" << v.id << " = s.static_var_" << v.static_arg->name
        << "_valid;" << endl;
  } else if (v.op == VAL_LD_IDX_STATIC) {
    if (is_sram_array(v.static_arg->t) && v.args.size() == 1) {
      out << "    val" << v.id << " = s.static_var_" << v.static_arg->name
          << '[' << arg_name(&b, v.args[0]) << "];" << endl;
    } else {
      out << "    // UNSUPPORTED TYPE FOR LD_IDX_STATIC" << endl;
    }
  } else if (v.op == VAL_ST_IDX_STATIC) {
    if (is_sram_array(v.static_arg->t) && v.args.size() == 2) {
      out << "    s.static_var_" << v.static_arg->name << '['
          << arg_name(&b, v.args[0]) << "] = " << arg_name(&b, v.args[1])
          << ';' << endl;
    } else {
      out << "    // UNSUPPORTED TYPE FOR ST_IDX_STATIC" << endl;
    }
  } else if (v.op == VAL_ST_IDX) {
    if (is_integer_type(v.t)) {
      out << "    val" << v.id << " = st_idx(" << arg_name(&b, v.args[0])
          << ", " << arg_name(&b, v.args[1]) << ", " << arg_name(&b, v.args[2]);
      if (v.args.size() == 4) out << ", " << arg_name(&b, v.args[3]);
      out << ");" << endl;
    } else if (is_struct(v.t) && v.args[1]->op == VAL_CONST) {
      string field_name(v.t.get_field_name(const_val(*v.args[1])));

      out << "    val" << v.id << " = " << arg_name(&b, v.args[0]) << ';' << endl
          << "    val" << v.id << '.' << field_name << " = "
          << arg_name(&b, v.args[2]) << ';' << endl;
    } else {
      out << "    // UNSUPPORTED TYPE FOR ST_IDX" << endl;
    }
  } else if (v.op == VAL_ST_STATIC) {
    out << "    s.";
    if (!v.static_arg->broadcast) out << "next_";
    out << "static_var_" << v.static_arg->name << " = "
        << arg_name(&b, v.args[0]) << ';' << endl;
    if (v.static_arg->broadcast)
      out << "    s.static_var_" << v.static_arg->name << "_valid = true;"
          << endl;
  } else if (v.op == VAL_LD_IDX) {
    if (is_integer_type(v.args[0]->t)) {
      out << "    val" << v.id << " = (" << arg_name(&b, v.args[0]) << " >> "
          << arg_name(&b, v.args[1]) << ')';
      if (v.args.size() > 2)
        out << " & ((1<<" << arg_name(&b, v.args[2]) << ")-1)";
      out << ';' << endl;
    } else if (is_struct(v.args[0]->t) && v.args[1]->op == VAL_CONST) {
      string field_name(v.t.get_field_name(const_val(*v.args[1])));
      out << "    val" << v.id << " = " << arg_name(&b, v.args[0]) << '.'
          << field_name << ';' << endl;
    } else if (is_static_array(v.args[0]->t)) {
      out << "    val" << v.id << " = " << arg_name(&b, v.args[0]) << '['
          << arg_name(&b, v.args[1]) << "];";
    } else {
      out << "    // UNSUPPORTED TYPE FOR LD_IDX" << endl;
    }
  } else if (v.op == VAL_SPAWN || v.op == VAL_CALL) {
    out << "    s.func" << v.id << "->call.valid = true;" << endl;
    for (unsigned i = 0; i < v.args.size(); ++i)
      out << "    s.func" << v.id << "->call.arg" << i << " = "
          << arg_name(&b, v.args[i]) << ';' << endl;

    // Calls must store live values on heap.
    if (v.op == VAL_CALL) {
      out << "    s.func" << v.id << "->call.live = new "
          << fname << "_bb" << b.id << "_out_t();" << endl;
      out << "    ((" << fname << "_bb" << b.id << "_out_t*)s.func"
          << v.id << "->call.live)->valid = true;" << endl
          << "    ((" << fname << "_bb" << b.id << "_out_t*)s.func"
          << v.id << "->call.live)->live = s.bb" << b.id << "_in.live;" << endl;
      for (auto &w : b.live_out) {
        out << "    ((" << fname << "_bb" << b.id << "_out_t*)s.func"
            << v.id << "->call.live)->val" << w->id
            << " = " << arg_name(&b, w) << ';' << endl;
      }
      out << "    s." << "bb" << b.id << "_in.valid = false;" << endl;
    }
  } else {
    if (v.args.size() == 2) {
      out << "    val" << v.id << " = "
          << arg_name(&b, v.args[0]) << ' ' << op_string(v.op, is_bit_type(v.t))
          << ' ' << arg_name(&b, v.args[1]) << ';' << endl;
    } else {
      out << "    // UNKNOWN" << endl;
    }
  }

  if (!is_void_type(v.t) && v.op != VAL_PHI && v.op != VAL_SPAWN) {
    out << "    std::cout << \"" << fname << v.id << " = \" << "
        << val_name.str() << " << std::endl;" << endl;
  }
}

static
  void gen_arbiter(std::ostream &out, std::string fname, if_func &f, if_bb &b)
{
  using namespace std;

  out << "  // arbiter for basic block" << b.id << endl;

  unsigned phi_arg_idx = 0;
  for (auto &p : b.pred) {
    map<int, int> phis;
    for (auto &v : b.vals)
      if (v->op == VAL_PHI)
        phis[v->id] = v->args[phi_arg_idx]->id;
    
    out << "  // input from bb" << p->id << endl;
    
    out << "  if (!s.bb" << b.id <<  "_in.valid && s.bb" << p->id
        << "_out.valid";
    if (p->suc.size() > 1)
      out << " && s.bb" << p->id << "_out.sel == " << which_suc(p, &b);
    out << ") {" << endl;

    // copy caller live values pointer
    out << "    s.bb" << b.id << "_in.live = s.bb" << p->id << "_out.live;"
        << endl;
    
    // copy live inputs
    for (auto &v : b.live_in) {
      out << "    s.bb" << b.id << "_in.val" << v->id << " = ";
      if (phis.count(v->id))
        out << "s.bb" << p->id << "_out.val" << phis[v->id] << ';' << endl;
      else
        out << "s.bb" << p->id << "_out.val" << v->id << ';' << endl;
    }
    out << "    s.bb" << b.id << "_in.valid = true;" << endl
        << "    s.bb" << p->id << "_out.valid = false;" << endl;
    
    out << "  }" << endl;

    phi_arg_idx++;
  }

  if (&b == f.bbs[0]) {
    out << "  // " << fname << " call input." << endl
        << "  if (!s.bb" << b.id << "_in.valid && s.call.valid) {" << endl
        << "    s.call.valid = false;" << endl
        << "    s.bb" << b.id << "_in.valid = true;" << endl
        << "    s.bb" << b.id << "_in.live = s.call.live;" << endl;
    for (unsigned i = 0; i < f.args.size(); ++i) {
      out << "    s.bb" << b.id << "_in.arg" << i << " = s.call.arg" << i
          << ';' << endl;
    }

    out << "  }" << endl;
  }
}

static bool has_side_effects(if_op o) {
  return o == VAL_ST_STATIC ||
         o == VAL_ST_IDX_STATIC ||
         o == VAL_CALL ||
         o == VAL_SPAWN;
}

static void gen_block(std::ostream &out, std::string fname, if_bb &b, std::map<bscotch::type, int> &m) {
  using namespace std;

  if_val *call = find_call_or_spawn(b);

  if (call) {
    out << "  tick_" << call->func_arg << "(*s.func"
        << call->id << ");" << endl;

    // Always ready for return from spawn.
    if (call->op == VAL_SPAWN)
      out << "  s.func" << call->id << "->ret.valid = false;" << endl;
  }
  
  out << "  // basic block " << b.id << endl
      << "  if (s.bb" << b.id << "_in.valid";
  if (call) {
    out << " && !s.func" << call->id << "->call.valid";
  }
  if (!call || call->op == VAL_SPAWN)
    out << " && !s.bb" << b.id << "_out.valid";
  out << ") {" << endl;

  // Generate all of the vals without side-effects
  unsigned vals_left = b.vals.size();
  for (auto &v : b.vals) {
    if (!has_side_effects(v->op)) {
      gen_val(out, fname, b, *v, m);
      --vals_left;
    }
  }

  // Generate a (possibly skippable) set of vals with side effects.
  if (/*vals_left*/1) { // use vals_left if we stop printing the running message
    if (b.stall)
      out << "    if (val" << b.stall->id << ") goto skip_stall_bb"
          << b.id << ';' << endl;

    out << "    std::cout << \"running " << fname << " basic block " << b.id
        << ", live = \" << s.bb" << b.id << "_in.live << std::endl;" << endl;
    
    for (auto &v : b.vals) {
      if (has_side_effects(v->op)) {
        gen_val(out, fname, b, *v, m);
        --vals_left;
      }
    }
  
    if (b.stall)
      out << "    skip_stall_bb" << b.id << ":;" << endl;
  }

  if (b.branch_pred) {
    out << "    s.bb" << b.id << "_out.sel = " << arg_name(&b, b.branch_pred)
        << ';' << endl;
  }

  out << "    // output connections" << endl;
  if (call && call->op == VAL_CALL) {
    out << "  }" << endl
        << "  if (!s.bb" << b.id << "_out.valid && s.func" << call->id
        << "->ret.valid) {" << endl
        << "    s.func" << call->id << "->ret.valid = false;" << endl
        << "    s.bb" << b.id << "_out = *((" << fname << "_bb"
        << b.id << "_out_t*)s.func" << call->id << "->ret.live);"
        << endl
        << "    delete (" << fname << "_bb" << b.id << "_out_t*)s.func"
        << call->id << "->ret.live;" << endl
        << "  }" << endl;
  } else {
    out << "    if (!s.bb" << b.id << "_out.valid && s.bb"
        << b.id << "_in.valid";
    if (b.stall) out << " && !" << arg_name(&b, b.stall);
    out << ") {" << endl;
    if (!find_ret(b))
      out << "      s.bb" << b.id << "_out.valid = true;" << endl;
    out << "      s.bb" << b.id << "_in.valid = false;" << endl
        << "      s.bb" << b.id << "_out.live = s.bb" << b.id << "_in.live;"
        << endl;

    for (auto &v : b.live_out) {
      out << "      s.bb" << b.id << "_out.val" << v->id << " = "
          << arg_name(&b, v) << ';' << endl;
    }
    out << "    }" << endl
        << "  }" << endl;
  }

  out << endl;
}

static void gen_func(std::ostream &out, std::string name, if_func &f, std::map<bscotch::type, int> &m) {
  using namespace std;

  out << "// " << name << " definitions." << endl
      << "void init_" << name << '(' << name << "_state_t &s) {" << endl;
  for (auto &b : f.bbs) {
    out << "  s.bb" << b->id << "_in.valid = false;" << endl
        << "  s.bb" << b->id << "_out.valid = false;" << endl;
    
    if_val *call = find_call_or_spawn(*b);
    if (call) {
      out << "  s.func" << call->id << " = new " << call->func_arg
          << "_state_t();" << endl
          << "  init_" << call->func_arg << "(*s.func" << call->id
          << ");" << endl;
    }
  }
  out << '}' << endl << endl;
  
  out << "void tick_" << name << "(" << name << "_state_t &s) {" << endl;

  for (auto &s : f.static_vars) {
    if (!s.second.broadcast && !is_sram_array(s.second.t))
      out << "  s.static_var_" << s.second.name << " = s.next_static_var_"
          << s.second.name << ';' << endl;
    else if (s.second.broadcast)
      out << "  s.static_var_" << s.second.name << "_valid = false;" << endl;
  }

  vector<if_bb*> bbs;
  
  order_blocks(bbs, f.bbs);
  
  for (auto &b : bbs)
    gen_arbiter(out, name, f, *b);

  for (auto &b : bbs)
    gen_block(out, name, *b, m);

  for (auto &s : f.static_vars) {
    out << "  std::cout << \"" << s.second.name << " = \" << s.static_var_"
        << s.second.name << " << std::endl;" << endl;
    if (s.second.broadcast)
      out << "  std::cout << \"" << s.second.name
          << "_valid = \" << s.static_var_" << s.second.name
          << "_valid << std::endl;" << endl;
  }
  
  out << '}' << endl;
}

static void gen_func_forward_decls
  (std::ostream &out, std::string name, if_func &f)
{
  using namespace std;

  out << "struct " << name << "_state_t;" << endl
      << "void init_" << name << "(" << name << "_state_t&);" << endl
      << "void tick_" << name << "(" << name << "_state_t&);" << endl;
}

static void gen_func_decls(std::ostream &out, std::string name, if_func &f, std::map<bscotch::type, int> &m) {
  using namespace std;

  out << "// " << name << " declarations." << endl;

  // Call and return types.
  out << "struct " << name << "_call_t {" << endl
      << "  bool valid;" << endl
      << "  void *live;" << endl;
  for (unsigned i = 0; i < f.args.size(); ++i) {
    ostringstream arg_name;
    arg_name << "arg" << i;
    out << "  " << type_cpp(f.args[i], m) << ' ' << arg_name.str() << ';' << endl;
  }
    
  out << "};" << endl << endl
      << "struct " << name << "_ret_t {" << endl
      << "  bool valid;" << endl
      << "  void *live;" << endl;
  if (!is_void_type(f.rtype))
    out << "  " << type_cpp(f.rtype, m) << " rval;" << endl;
  out << "};" << endl << endl;
  
  // Input and output types for every basic block.
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    out << "struct " << name << "_bb" << i << "_in_t {" << endl
        << "  bool valid;" << endl
        << "  void *live;" << endl;
    if (i == 0)
      for (unsigned j = 0; j < f.args.size(); ++j)
        out << "  " << type_cpp(f.args[j], m) << " arg" << j << ';' << endl;
         
    for (unsigned j = 0; j < f.bbs[i]->live_in.size(); ++j) {
      if_val *v = f.bbs[i]->live_in[j];
      ostringstream var_name;
      var_name << "val" << v->id;
      out << "  " << type_cpp(v->t, m) << ' ' << var_name.str() << ';' << endl;
    }
    out << "};" << endl << endl;

    out << "struct " << name << "_bb" << i << "_out_t {" << endl
        << "  bool valid;" << endl
        << "  void *live;" << endl;
    if (f.bbs[i]->suc.size() > 1) out << "  int sel;" << endl;
    for (unsigned j = 0; j < f.bbs[i]->live_out.size(); ++j) {
      if_val *v = f.bbs[i]->live_out[j];
      ostringstream var_name;
      var_name << "val" << v->id;
      out << "  " << type_cpp(v->t, m) << ' ' << var_name.str() << ';' << endl;
    }
    out << "};" << endl << endl;
  }

  // State type containing all blocks' inputs/outputs.
  out << "struct " << name << "_state_t {" << endl
      << "  " << name << "_call_t call;" << endl
      << "  " << name << "_ret_t ret;" << endl;
  for (auto &v : f.static_vars) {
    out << "  " << type_cpp(v.second.t, m)
        << " static_var_" << v.second.name;
    if (!v.second.broadcast && !is_sram_array(v.second.t))
      out << ", next_static_var_" << v.second.name;
    out << ';' << endl;

    if (v.second.broadcast)
      out << "  bool static_var_" << v.second.name << "_valid;" << endl;
  }
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    out << "  " << name << "_bb" << i << "_in_t bb" << i << "_in";
    out << ';' << endl;
    out << "  " << name << "_bb" << i << "_out_t bb" << i << "_out;" << endl;
    if_val *call = find_call_or_spawn(*f.bbs[i]);
    if (call)
      out << "  " << call->func_arg << "_state_t *func"
          << call->id << ';' << endl;
  }
  out << "};" << endl << endl;
}

void bscotch::gen_prog_cpp(std::ostream &out, if_prog &p) {
  using namespace std;
  map<bscotch::type, int> m;
  catalog_types(m, p);
  gen_struct_decls(out, m);
  show_catalog(m);
  
  out << "// Forward declarations." << std::endl;
  for (auto &f : p.functions)
    gen_func_forward_decls(out, f.first, f.second);
  out << std::endl;
  
  for (auto &f : p.functions)
    gen_func_decls(out, f.first, f.second, m);

  for (auto &f : p.functions)
    gen_func(out, f.first, f.second, m);
}
