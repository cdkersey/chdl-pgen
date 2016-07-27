#include <iostream>

#include <map>
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

static const char *op_string(if_op op) {
  switch(op) {
  case VAL_NEG: return "-";
  case VAL_NOT: return "~";
  case VAL_ADD: return "+";
  case VAL_SUB: return "-";
  case VAL_MUL: return "*";
  case VAL_DIV: return "/";
  case VAL_AND: return "&";
  case VAL_OR: return "|";
  case VAL_XOR: return "^";
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

static void gen_val(std::ostream &out, std::string fname, if_bb &b, if_val &v) {
  using namespace std;

  ostringstream val_name;
  val_name << "val" << v.id;
  if (!is_void_type(v.t) && v.op != VAL_PHI && v.op != VAL_SPAWN)
    out << "    " << type_cpp(v.t) << ' ' << val_name.str() << ';' << endl;
  
  if (v.op == VAL_PHI) {
    // Handled in arbiter.
  } else if (v.op == VAL_NEG || v.op == VAL_NOT) {
    out << "    val" << v.id << " = " << op_string(v.op)
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
    out << "    cat(val" << v.id << ')';
    for (unsigned i = 0; i < v.args.size(); ++i)
      out << '(' << arg_name(&b, v.args[i]) << ')';
    out << ';' << endl;
  } else if (v.op == VAL_ARG) {
    out << "    val" << v.id << " = s.call.arg" << v.static_access_id << ';'
        << endl;
  } else if (v.op == VAL_RET) {
    out << "    if (!s.ret.valid) {" << endl;
    if (v.args.size() && !is_void_type(v.args[0]->t))
      out << "      s.ret.rval = " << arg_name(&b, v.args[0]) << ';';
    out << "      s.ret.valid = true;" << endl
        << "    }" << endl;
  } else if (v.op == VAL_LD_STATIC) {
    out << "    val" << v.id << " = s.static_var_" << v.static_arg->name << ';'
        << endl;
  } else if (v.op == VAL_ST_STATIC) {
    out << "    s.";
    if (!v.static_arg->broadcast) out << "next_";
    out << "static_var_" << v.static_arg->name << " = "
        << arg_name(&b, v.args[0]) << ';' << endl;
  } else if (v.op == VAL_SPAWN) {
    // TODO: copy args, etc.
  } else {
    if (v.args.size() == 2) {
      out << "    val" << v.id << " = "
          << arg_name(&b, v.args[0]) << ' ' << op_string(v.op)
          << ' ' << arg_name(&b, v.args[1]) << ';' << endl;
    } else {
      out << "    // UNKNOWN" << endl;
    }
  }
}

static
  void gen_arbiter(std::ostream &out, std::string fname, if_func &f, if_bb &b)
{
  using namespace std;

  out << "  // arbiter for basic block" << b.id << endl;

  if (b.cycle_breaker) {
    out << "  if (!s.bb" << b.id << "_in.valid && s.bb" << b.id
        << "_in_tmp.valid) {" << endl
        << "    s.bb" << b.id << "_in = s.bb" << b.id << "_in_tmp;" << endl
        << "    s.bb" << b.id << "_in_tmp.valid = false;" << endl
        << "  }" << endl;
  }
  
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

    if (b.cycle_breaker) {
      out << "  if (!s.bb" << b.id <<  "_in_tmp.valid && s.bb" << p->id
          << "_out.valid";
      if (p->suc.size() > 1)
        out << " && s.bb" << p->id << "_out.sel == " << which_suc(p, &b);
      out << ") {" << endl;

      // copy live inputs
      for (auto &v : b.live_in) {
        out << "    s.bb" << b.id << "_in_tmp.val" << v->id << " = ";
        if (phis.count(v->id))
          out << "s.bb" << p->id << "_out.val" << phis[v->id] << ';' << endl;
        else
          out << "s.bb" << p->id << "_out.val" << v->id << ';' << endl;
      }
      out << "    s.bb" << b.id << "_in_tmp.valid = true;" << endl
          << "    s.bb" << p->id << "_out.valid = false;" << endl;
    
      out << "  }" << endl;      
    }

    phi_arg_idx++;
  }

  if (&b == f.bbs[0]) {
    out << "  // " << fname << " call input." << endl
        << "  if (!s.bb" << b.id << "_in.valid && s.call.valid) {" << endl
        << "    s.call.valid = false;" << endl
        << "    s.bb" << b.id << "_in.valid = true;" << endl
        << "  }" << endl;
    if (b.cycle_breaker) {
      out << "  if (!s.bb" << b.id << "_in_tmp.valid && s.call.valid) {" << endl
          << "    s.call.valid = false;" << endl
          << "    s.bb" << b.id << "_in_tmp.valid = true;" << endl
          << "  }" << endl;
    }
  }

}

static void gen_block(std::ostream &out, std::string fname, if_bb &b) {
  using namespace std;

  if_val *call = find_call_or_spawn(b);
  
  out << "  // basic block " << b.id << endl
      << "  if (s.bb" << b.id << "_in.valid";
  if (call) {
    out << " && /* can make a call */";
  }
  out << ") {" << endl;

  for (auto &v : b.vals)
    gen_val(out, fname, b, *v);

  if (b.branch_pred) {
    out << "    s.bb" << b.id << "_out.sel = " << arg_name(&b, b.branch_pred)
        << ';' << endl;
  }

  out << "    // output connections" << endl
      << "    if (!s.bb" << b.id << "_out.valid && s.bb"
      << b.id << "_in.valid";
  if (b.stall) out << " && !" << arg_name(&b, b.stall);
  out << ") {" << endl
      << "      s.bb" << b.id << "_out.valid = true;" << endl
      << "      s.bb" << b.id << "_in.valid = false;" << endl;

  for (auto &v : b.live_out) {
    out << "      s.bb" << b.id << "_out.val" << v->id << " = "
        << arg_name(&b, v) << ';' << endl;
  }
  out << "    }" << endl
      << "  }" << endl;

  out << endl;
}

static void gen_func(std::ostream &out, std::string name, if_func &f) {
  using namespace std;

  out << "// " << name << " definition." << endl
      << "void tick_" << name << "(" << name << "_state_t &s) {" << endl;

  for (auto &s : f.static_vars) {
    if (!s.second.broadcast)
      out << "  s.static_var_" << s.second.name << " = s.next_static_var_"
          << s.second.name << ';' << endl;
    out << "  std::cout << \"" << s.second.name << " = \" << s.static_var_"
        << s.second.name << " << std::endl;" << endl;
  }
  
  for (auto &b : f.bbs)
    gen_arbiter(out, name, f, *b);
  
  for (auto &b : f.bbs)
    gen_block(out, name, *b);
  
  out << '}' << endl;  
}

static void gen_func_decls(std::ostream &out, std::string name, if_func &f) {
  using namespace std;

  out << "// " << name << " declarations." << endl;

  // Call and return types.
  out << "struct " << name << "_call_t {" << endl
      << "  bool valid;" << endl
      << "  void *live;" << endl;
  for (unsigned i = 0; i < f.args.size(); ++i) {
    ostringstream arg_name;
    arg_name << "arg" << i;
    out << "  " << type_cpp(f.args[i]) << ' ' << arg_name.str() << ';' << endl;
  }
    
  out << "};" << endl << endl
      << "struct " << name << "_ret_t {" << endl
      << "  bool valid;" << endl
      << "  void *live;" << endl;
  if (!is_void_type(f.rtype))
      out << "  " << type_cpp(f.rtype) << " rval;" << endl;
  out << "};" << endl << endl;
  
  // Input and output types for every basic block.
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    out << "struct " << name << "_bb" << i << "_in_t {" << endl
        << "  bool valid;" << endl
        << "  void *live;" << endl;
    for (unsigned j = 0; j < f.bbs[i]->live_in.size(); ++j) {
      if_val *v = f.bbs[i]->live_in[j];
      ostringstream var_name;
      var_name << "val" << v->id;
      out << "  " << type_cpp(v->t) << ' ' << var_name.str() << ';' << endl;
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
      out << "  " << type_cpp(v->t) << ' ' << var_name.str() << ';' << endl;
    }
    out << "};" << endl << endl;
  }

  // State type containing all blocks' inputs/outputs.
  out << "struct " << name << "_state_t {" << endl
      << "  " << name << "_call_t call;" << endl
      << "  " << name << "_ret_t ret;" << endl;
  for (auto &v : f.static_vars) {
    out << "  " << type_cpp(v.second.t)
        << " static_var_" << v.second.name;
    if (!v.second.broadcast) out << ", next_static_var_";
    out << v.second.name << ';' << endl;
  }
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    out << "  " << name << "_bb" << i << "_in_t bb" << i << "_in";
    if (f.bbs[i]->cycle_breaker)
      out << ", bb" << i << "_in_tmp";
    out << ';' << endl;
    out << "  " << name << "_bb" << i << "_out_t bb" << i << "_out;" << endl;
  }
  out << "};" << endl << endl;

  // Function declaration.
  out << "void tick_" << name << "(" << name << "_state_t &s);" << endl << endl;
}

void bscotch::gen_prog_cpp(std::ostream &out, if_prog &p) {
  for (auto &f : p.functions)
    gen_func_decls(out, f.first, f.second);

  for (auto &f : p.functions)
    gen_func(out, f.first, f.second);
}