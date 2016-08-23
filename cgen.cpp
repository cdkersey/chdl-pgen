#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#include "type.h"
#include "if.h"
#include "cgen.h"

using namespace pgen;

namespace pgen {
  static void gen_val(std::ostream &out, std::string fname, int bbidx, int idx, if_bb &b, if_val &v);
  static void gen_bb_decls(std::ostream &out, std::string fname, int idx, if_bb &b, bool entry);
  static void gen_bb(std::ostream &out, std::string fname, int idx, if_bb &b, bool entry);
  static void gen_func_decls(std::ostream &out, std::string name, if_func &f);
  static void gen_func(std::ostream &out, std::string name, if_func &f);
}

static bool is_store(if_op op) {
  return op == VAL_ST_STATIC || op == VAL_ST_IDX || op == VAL_ST_IDX_STATIC;
}

static bool is_load(if_op op) {
  return op == VAL_LD_STATIC || op == VAL_LD_IDX || op == VAL_LD_IDX_STATIC;
}

static void gen_static_var(
  std::ostream &out, if_staticvar &s, std::string fname,
  unsigned loads, unsigned stores)
{
  using std::endl;

  if (s.broadcast) {
    out << "  BCAST_VAR(" << fname << ", " << s.name << ", " << type_chdl(s.t)
        << ");" << endl;
  } else if (is_sram_array(s.t)) {
    out << "  staticarray<" << type_chdl(element_type(s.t)) << ", CLOG2("
        << array_len(s.t) << "), " << loads << "> " << fname << "_" << s.name
        <<';' << endl;
  } else {
    out << "  staticvar<" << type_chdl(s.t) << ", " << stores
        << "> " << fname << '_' << s.name << "(0x" << to_hex(s.initial_val)
        << ");" << endl
        << "  tap(\"" << fname << "_" << s.name << "\", " << fname << '_'
        << s.name << ".value());" << endl;
  }
}

static void gen_static_var_bottom
  (std::ostream &out, if_staticvar &s, std::string fname)
{
  using std::endl;
  out << "  STATIC_VAR_GEN(" << fname << ", " << s.name << ");" << endl;
}

static bool is_binary(if_op o) {
  return (o == VAL_ADD) || (o == VAL_SUB) || (o == VAL_MUL) || (o == VAL_DIV) ||
         (o == VAL_AND) || (o == VAL_OR) || (o == VAL_XOR) || (o == VAL_EQ) ||
         (o == VAL_LT);
}

static bool is_unary(if_op o) {
  return (o == VAL_NEG) || (o == VAL_NOT);
 }

static bool is_reduce(if_op o) {
  return (o == VAL_AND_REDUCE) || (o == VAL_OR_REDUCE);
}

static bool type_is_bit(const type &t) {
  return t.type_vec.size() == 1 && t.type_vec[0] == TYPE_BIT;
}

static std::string op_str(if_op o, const type &t, const type &u) {
  bool bit(type_is_bit(t) && type_is_bit(u));
  switch (o) {
  case VAL_ADD: return bit ? " != " : " + ";
  case VAL_SUB: return bit ? " != " : " - ";
  case VAL_MUL: return bit ? " && " : " * ";
  case VAL_DIV: return bit ? " && " : " / ";
  case VAL_AND: return bit ? " && " : " & ";
  case VAL_OR:  return bit ? " || " : " | ";
  case VAL_XOR: return bit ? " != " : " ^ ";
  case VAL_EQ:  return " == ";
  case VAL_LT:  return " < ";
  };

  return " UNSUPPORTED OP ";
}

static bool val_is_1cyc(const if_val &v) {
  return v.op == VAL_LD_IDX_STATIC;
}

static std::string op_str(if_op o, const type &t) {
  bool bit(type_is_bit(t));

  if (o == VAL_NOT) return bit ? "!" : "~";
  else if (o == VAL_NEG) return bit ? "Lit(1) ||" : "-";
  else if (o == VAL_AND_REDUCE) return "AndN";
  else if (o == VAL_OR_REDUCE) return "OrN";

  return " UNSUPPORTED OP ";
 }

static std::string val_name(std::string fname, int bbidx, if_bb &b, if_val &v)
{
  std::ostringstream oss;
  bool in_block = false;
  for (auto &y : b.vals)
    if (y->id == v.id && v.op != VAL_PHI) in_block = true;

  if (in_block)
    oss << fname << '_' << v.id;
  else
    oss << "_(_(" << fname << "_bb" << bbidx << "_in, \"contents\"), \"val"
        << v.id << "\")";
  return oss.str();
}

static void pgen::gen_val(std::ostream &out, std::string fname, int bbidx, int idx, if_bb &b, if_val &v)
{
  using std::endl;
  using std::string;
  using std::vector;
  using std::ostringstream;
  out << "  ";

  if (!is_store(v.op) && v.op != VAL_PHI && v.op != VAL_CALL
      && v.op != VAL_RET && v.op != VAL_SPAWN && v.op != VAL_CONCATENATE
      && v.op != VAL_BUILD && v.op != VAL_SELECT)
  {
    out << type_chdl(v.t) << ' ' << fname << '_' << v.id << " = ";
  }

  vector<string> aname;
  for (auto &x : v.args) {
    aname.push_back(val_name(fname, bbidx, b, *x));
  }
  
  if (v.op == VAL_CONST) {
    if (is_struct(v.t)) {
      out << type_chdl(v.t) << "(Lit<" << v.t.size() << ">(0x"
          << to_hex(v.const_val) << "))";
    } else if (is_bit_type(v.t)) {
      out << "Lit(" << v.const_val[0] << ')';
    } else {
      out << "Lit<" << v.t.size() << ">(0x" << to_hex(v.const_val) << ')';
    }
  } else if (v.op == VAL_LD_STATIC) {
    out << "  LD_STATIC(" << fname << ", " << v.static_arg->name << ')';
  } else if (is_binary(v.op)) {
    out << aname[0] << op_str(v.op, v.args[0]->t, v.args[1]->t) << aname[1];
  } else if (is_unary(v.op)) {
    out << op_str(v.op, v.args[0]->t) << '(' << aname[0] << ')';
  } else if (is_reduce(v.op)) {
    out << op_str(v.op, v.args[0]->t) << '(' << aname[0] << ')';
  } else if (v.op == VAL_CONCATENATE) {
    out << type_chdl(v.t) << ' ' << fname << '_' << v.id << ';' << endl
        << "  Flatten(" << fname << '_' << v.id << ") = ";
    if (v.args.size() == 1) {
      out << "Flatten(" << aname[0] << ')';
    } else {
      for (unsigned i = 0; i < v.args.size(); ++i) {
        out << "Cat(Flatten(" << aname[i] << "))";
        if (i != v.args.size() - 1) out << '.';
      }
    }
  } else if (v.op == VAL_BUILD) {
    out << "  " << type_chdl(v.t) << ' ' << fname << '_' << v.id << ';' << endl;
    
    if (is_struct(v.t)) {
      for (unsigned i = 0; i < v.args.size(); ++i) {
        out << "  Flatten(_(" << fname << '_' << v.id << ", \""
            << v.t.get_field_name(i) << "\")) = Flatten("
            << val_name(fname, bbidx, b, *v.args[i]) << ");" << endl;
      }
    } else {
      for (unsigned i = 0; i < v.args.size(); ++i) {
        out << "  Flatten(" << fname << '_' << v.id << '[' << i << "]) = "
            << "Flatten(" << val_name(fname, bbidx, b, *v.args[i]) << ");"
            << endl;
      }
    }
  } else if (v.op == VAL_SELECT) {
    out << "vec<" << v.args.size() - 1 << ", " << type_chdl(v.t) << " > sel"
        << v.id << "_inputs;" << endl
        << "  " << type_chdl(v.t) << ' ' << fname << '_' << v.id
        << " = Mux(" << val_name(fname, bbidx, b, *v.args[0])
        << ", sel" << v.id << "_inputs);" << endl;

    for (unsigned i = 1; i < v.args.size(); ++i) {
      out << "  sel" << v.id << "_inputs[" << i - 1 << "] = "
          << val_name(fname, bbidx, b, *v.args[i]);
      if (i != v.args.size() - 1) out << ';' << endl;
    }
  } else if (v.op == VAL_ARG) {
    out << "_(_(_(" << fname << "_bb0_in, \"contents\"), \"args\"), \"arg"
        << v.static_access_id << "\")";
  } else if (v.op == VAL_ST_STATIC) {
    ostringstream preds;
    if (v.pred) {
      preds << " && " << val_name(fname, bbidx, b, *v.pred);
    }

    out << "ST_STATIC(" << fname << ", " << v.static_arg->name << ", "
        << aname[0] << ", "
        << fname << "_bb" << bbidx << "_run" << preds.str() << ", "
        << v.static_access_id << ')';
  } else if (v.op == VAL_PHI) {
    // Do nothing; phis are handled entirely in the inputs to the block.
  } else if (v.op == VAL_LD_IDX) {
    if (is_struct(v.args[0]->t)) {
      if (v.args[1]->op != VAL_CONST) {
	std::cout << "Attempt to index struct with non-const." << endl;
	abort();
      }

      string fieldname = v.args[0]->t.get_field_name(const_val(*v.args[1]));

      out << "_(" << aname[0] << ", \"" << fieldname << "\")";
    } else if (v.args.size() == 3 &&
               v.args[1]->op == VAL_CONST && v.args[2]->op == VAL_CONST)
    {
      unsigned idx0(const_val(*v.args[1])), len(const_val(*v.args[2]));
      out << aname[0] << "[range<" << idx0 << ", " << idx0 + len - 1 << ">()]";
    } else if (v.args.size() == 3 && v.args[2]->op == VAL_CONST) {
      unsigned len(const_val(*v.args[2]));
      out << "MultiMux<" << len << ">(" << aname[1] << ", " << aname[0] << ')';
    } else if (v.args.size() == 2 && v.args[1]->op == VAL_CONST) {
      out << aname[0] << '[' << const_val(*v.args[1]) << ']';
    } else {
      out << "Mux(" << aname[1] << ", " << aname[0] << ')';
    }
  } else if (v.op == VAL_ST_IDX) {
    if (is_struct(v.args[0]->t)) {
      if (v.args[1]->op != VAL_CONST) {
	std::cout << "Attempt to index struct with non-const." << endl;
	abort();
      }
      string fieldname = v.args[0]->t.get_field_name(const_val(*v.args[1]));

      out << type_chdl(v.t) << ' ' << fname << '_' << v.id << ';' << endl
	  << "  Flatten(" << fname << '_' << v.id << ") = ~~Flatten("
          << aname[0] << ");" << endl
          << "  _(" << fname << '_' << v.id << ", \"" << fieldname << "\") = "
          << aname[2] << ';' << endl;
    } else if (v.args.size() == 4 &&
               v.args[1]->op == VAL_CONST && v.args[2]->op == VAL_CONST)
    {
      unsigned idx(const_val(*v.args[1])), len(const_val(*v.args[2]));
      out << type_chdl(v.t) << ' ' << fname << '_' << v.id << ';' << endl
          << "  Flatten(" << fname << '_' << v.id << ") = ~~Flatten("
          << aname[0] << ");" << endl
          << "  " << fname << '_' << v.id << "[range<" << idx << ", "
          << idx + len - 1 << ">()]" << " = " << aname[3] << ';';
    } else if (v.args.size() == 4 && v.args[2]->op == VAL_CONST) {
      out << type_chdl(v.t) << ' ' << fname << '_' << v.id << ';' << endl
          << "  Flatten(" << fname << '_' << v.id << ") = ~~Flatten("
          << aname[0] << ");" << endl
          << "  MultiRepl(" << fname << '_' << v.id << ", " << aname[1] << ", "
          << aname[3] << ");" << endl;
    } else if (v.args.size() == 3 && v.args[2]->op == VAL_CONST) {
      out << type_chdl(v.t) << ' ' << fname << '_' << v.id << ';' << endl
          << "  Flatten(" << fname << '_' << v.id << ") = ~~Flatten("
          << aname[0] << ");" << endl
          <<  "  " << fname << '_' << v.id << '[' << const_val(*v.args[1])
          << "] = " << aname[2] << endl;
    } else if (v.args.size() == 3) {
      out << type_chdl(v.t) << ' ' << fname << '_' << v.id << " = "
	  << "SingleRepl(" << aname[1] << ", " << aname[0] << ", " << aname[2]
          << ");" << endl;
    }
  } else if (v.op == VAL_LD_IDX_STATIC) {
    out << "LD_STATIC_ARRAY(" << fname << ", " << v.static_arg->name << ", "
        << aname[0] << ", " << v.static_access_id << ')';
  } else if (v.op == VAL_ST_IDX_STATIC) {
    ostringstream preds;
    if (v.pred) preds << " && " << val_name(fname, bbidx, b, *v.pred);
    out << "ST_STATIC_ARRAY(" << fname << ", " << v.static_arg->name << ", "
        << aname[0] << ", " << aname[1] << ", " << fname << "_bb" << bbidx
        << "_run" << preds.str() << ')';
  } else if (v.op == VAL_BCAST_VALID_STATIC) {
    out << "LD_BCAST_VALID(" << fname << ", " << v.static_arg->name << ')';
  } else if (v.op == VAL_CALL || v.op == VAL_SPAWN) {
    ostringstream ctype_oss, rtype_oss, live_oss;

    if (v.op == VAL_CALL) {
      live_oss << fname << "_bb" << bbidx << "_out_contents_t";
    } else {
      live_oss << "chdl_void";
    }
    
    ctype_oss << v.func_arg << "_call_t<" << live_oss.str() << " >";
    rtype_oss << v.func_arg << "_ret_t<" << live_oss.str() << " >";
    
    string rtype = rtype_oss.str(),
           ctype = ctype_oss.str();

    out << rtype << ' ' << fname << "_call_" << v.id << "_ret;" << endl
        << "  " << ctype << ' ' << fname << "_call_" << v.id << "_args;" << endl
        << "  TAP(" << fname << "_call_" << v.id << "_ret);" << endl
        << "  TAP(" << fname << "_call_" << v.id << "_args);" << endl;

    if (v.op == VAL_CALL) {
      out << "  _(" << fname << "_call_" << v.id << "_args, \"valid\") = _("
          << fname << "_bb" << bbidx << "_in, \"valid\")";
      if (b.stall) out << " && !" << val_name(fname, bbidx, b, *b.stall);
      out << ';' << endl;
      out << "  _(" << fname << "_call_" << v.id << "_ret, \"ready\") = _("
          << fname << "_bb" << bbidx << "_out_prebuf, \"ready\");" << endl;
    } else {
      out << "  _(" << fname << "_call_" << v.id << "_args, \"valid\") = "
          << fname << "_bb" << bbidx << "_run;" << endl;
      out << "  _(" << fname << "_call_" << v.id << "_ret, \"ready\") = Lit(1);"
          << endl;
    }

    for (unsigned i = 0; i < v.args.size(); ++i) {
      out << "  _(_(" << fname << "_call_" << v.id
          << "_args, \"contents\"), \"arg" << i << "\") = "
          << aname[i] << ';' << endl;
    }

    if (v.op == VAL_CALL)
      out << "_(_(" << fname << "_call_" << v.id
          << "_args, \"contents\"), \"live\") = "
          << fname << "_bb" << bbidx << "_live;" << endl;

    out << "  " << v.func_arg << '(' << fname << "_call_" << v.id << "_ret, "
        << fname << "_call_" << v.id << "_args);" << endl;

    if (v.op == VAL_CALL)
      out << "  " << type_chdl(v.t) << ' ' << fname << '_' << v.id << " = "
          << "_(_(" << fname << "_call_" << v.id
          << "_ret, \"contents\"), \"rval\");" << endl;
  } else if (v.op == VAL_RET) {
    out << "  _(" << fname << "_ret, \"valid\") = _("
        << fname << "_bb" << bbidx << "_out_prebuf, \"valid\");" << endl
        << "  _(" << fname << "_bb" << bbidx << "_out_prebuf, \"ready\") = _("
        << fname << "_ret, \"ready\");" << endl;
    if (aname.size() > 0)
       out << "  _(_(" << fname << "_ret, \"contents\"), \"rval\") = "
           << aname[0] << ';' << endl;
    out << "  _(_(" << fname << "_ret, \"contents\"), \"live\") = _(_("
        << fname << "_bb" << bbidx << "_out_prebuf, \"contents\"), \"live\");"
        << endl;
  } else {
    out << "UNSUPPORTED_OP " << if_op_str[v.op];
  }

  if (v.op != VAL_PHI && v.op != VAL_CALL
      && v.op != VAL_RET && v.op != VAL_SPAWN)
  {
    out << ';' << endl;
  }
}

static void print_arg_type(std::ostream &out, std::vector<type> &v) {
  for (unsigned i = 0; i < v.size(); ++i) {
    out << "ag<STP(\"" << "arg" << i << "\"), " << type_chdl(v[i]);
    if (i != v.size() - 1) out << ", ";
  }
  for (unsigned i = 0; i < v.size(); ++i)
    out << " >";
}

static void print_live_type(std::ostream &out, std::vector<if_val*> &v) {
  if (v.size() == 0) {
    out << "chdl_void";
  } else {
    for (unsigned i = 0; i < v.size(); ++i) {
      out << "ag<STP(\"" << "val" << v[i]->id << "\"), ";
      if (val_is_1cyc(*v[i])) out << "late<" << type_chdl(v[i]->t) << " >";
      else out << type_chdl(v[i]->t);
      if (i != v.size() - 1) out << ", ";
    }
    for (unsigned i = 0; i < v.size(); ++i)
      out << " >";
  }
}

static std::string input_signal(std::string fname, int bbidx, int vidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_arb_in[" <<  vidx << "], \""
      << signal << "\")";
  return oss.str();
}

static std::string input_csignal(std::string fname, int bbidx, int vidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(_(" << fname << "_bb" << bbidx << "_arb_in[" <<  vidx << "], \"contents\"), \""
      << signal << "\")";
  return oss.str();
}

static std::string input_signal(std::string fname, int bbidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_in, \"" << signal << "\")";
  return oss.str();
}

static std::string output_signal(std::string fname, int bbidx, int vidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_out[" <<  vidx << "], \""
      << signal << "\")";
  return oss.str();
}

static std::string output_csignal(std::string fname, int bbidx, int vidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(_(" << fname << "_bb" << bbidx << "_out[" <<  vidx
      << "], \"contents\"), \"" << signal << "\")";
  return oss.str();
}

static std::string output_csignal(std::string fname, int bbidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_live, \"" << signal << "\")";
  return oss.str();
}

static std::string output_signal(std::string fname, int bbidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_out_prebuf, \"" << signal
      << "\")";
  return oss.str();
}

static void pgen::gen_bb_decls(std::ostream &out, std::string fname, int idx, if_bb &b, bool entry) {
  using std::endl;

  out << "  // " << fname << " BB " << idx << " declarations" << endl;
  out << "  hierarchy_enter(\"" << fname << "_bb" << idx << "_decls\");" << endl;
  // Typedef input/output types
  out << "  typedef ag<STP(\"live\"), OPAQUE";
  if (b.live_in.size() > 0) {
    out << ", ";
    print_live_type(out, b.live_in);
  }
  if (entry) out << ", ag<STP(\"args\"), " << fname << "_call_contents_t<OPAQUE> > ";
  out << " > " << fname << "_bb" << idx << "_in_contents_t;" << endl
      << "  typedef flit<" << fname << "_bb" << idx << "_in_contents_t> "
      << fname << "_bb" << idx << "_in_t;" << endl;

  out << "  typedef ag<STP(\"live\"), OPAQUE";
  if (b.live_out.size() > 0) {
    out << ", ";
    print_live_type(out, b.live_out);
  }
  out << " > " << fname << "_bb" << idx << "_out_contents_t;" << endl
      << "  typedef flit<" << fname << "_bb" << idx << "_out_contents_t> "
      << fname << "_bb" << idx << "_out_t;" << endl;
  
  // Declare input/output arrays
  out << "  " << fname << "_bb" << idx << "_in_t "
      << fname << "_bb" << idx << "_in_prebuf, "
      << fname << "_bb" << idx << "_in;" << endl;
  out << "  TAP(" << fname << "_bb" << idx << "_in);" << endl;
  out << "  TAP(" << fname << "_bb" << idx << "_in_prebuf);" << endl;

  int n_suc = b.suc.size();
  out << "  " << fname << "_bb" << idx << "_out_t " << fname << "_bb" << idx
      << "_out_prebuf;" << endl
      << "  " << fname << "_bb" << idx << "_out_contents_t " << fname << "_bb"
      << idx << "_live;" << endl
      << "  vec<" << n_suc << ", " << fname << "_bb" << idx << "_out_t> "
      << fname << "_bb" << idx << "_out;" << endl;
  out << "  TAP(" << fname << "_bb" << idx << "_out_prebuf);" << endl;

  int n_pred = b.pred.size();
  if (entry) n_pred++;
  out << "  vec<" << n_pred << ", " << fname << "_bb" << idx << "_in_t> "
      << fname << "_bb" << idx << "_arb_in;" << endl
      << "  TAP(" << fname << "_bb" << idx << "_arb_in);" << endl;

  out << "  hierarchy_exit();" << endl << endl;
}

static void get_val_map(std::map<int, int> &m, int pred_idx, if_bb &b) {
  // The only ones that are not 1-1 from the live_in/live_out sets are the
  // ones corresponding to phis. Scan the destination BB for phis, and insert
  // accordingly.
  for (auto &v : b.vals) {
    if (v->op == VAL_PHI) {
      int src = -1;
      if_bb &a = *(b.pred[pred_idx]);
      if_val *s = v->args[pred_idx];
      for (auto &lo : a.live_out) {
        if (lo->id == s->id) {
          if (src != -1) {
            std::cerr << "2 phi inputs: " << src << " and " << lo->id
                      << " from same pred block!" << std::endl;
            std::abort();
          }
          src = lo->id;
        }
      }
      m[v->id] = src;
    }
  }

  // The non-phis are all just 1-1 copies.
  for (auto &x : b.live_in)
    if (!m.count(x->id)) m[x->id] = x->id;
}

void print_spawn_readys(std::ostream &out, std::string fname, if_bb &b) {
  using namespace std;
  vector<if_val*> spawns;
  for (auto &v : b.vals)
    if (v->op == VAL_SPAWN)
      spawns.push_back(v);

  if (spawns.size()) {
    out << " && (";
    for (unsigned i = 0; i < spawns.size(); ++i) {
      if_val *p = spawns[i];
      out << "_(" << fname << "_call_" << p->id << "_args, \"ready\")";
      if (i != spawns.size() - 1)
        out << " || ";
    }
    out << ')';
  }
}

static void pgen::gen_bb(std::ostream &out, std::string fname, int idx, if_bb &b, bool entry)
{
  using std::endl;
  using std::map;

  // Find call if it exists
  if_val *call = NULL;
  for (auto &v : b.vals)
    if (v->op == VAL_CALL || v->op == VAL_SPAWN) call = v;
  
  out << "  // " << fname << " BB " << idx << " body" << endl;
  out << "  hierarchy_enter(\"" << fname << "_bb" << idx << "\");" << endl;  
  int n_suc = b.suc.size(), n_pred = b.pred.size();
  
  // Set up arbiter inputs
  for (unsigned i = 0; i < b.pred.size(); ++i) {
    map<int, int> vmap;
    get_val_map(vmap, i, b);

    int pred_id(b.pred[i]->id), pred_sidx = -1;
    for (unsigned j = 0; j < b.pred[i]->suc.size(); ++j) {
      if (b.pred[i]->suc[j] == &b) {
	pred_sidx = j;
      }
    }

    if (pred_sidx == -1) abort();
    
    out << "  " << input_signal(fname, idx, i, "valid")
        << " = " << output_signal(fname, b.pred[i]->id, pred_sidx, "valid")
        << ';' << endl
        << "  " << output_signal(fname, b.pred[i]->id, pred_sidx, "ready")
        << " = "
        << input_signal(fname, idx, i, "ready") << ';' << endl;

    for (auto &x : vmap) {
      out << "  // BB" << b.pred[i]->id << " val " << x.second << " -> BB "
	  << b.id << " val " << x.first << endl;
      std::ostringstream namep, name;
      namep << "val" << x.second;
      name << "val" << x.first;
      out << "  " << input_csignal(fname, idx, i, name.str()) << " = "
          << output_csignal(fname, b.pred[i]->id, pred_sidx, namep.str())
          << ';' << endl;
    }

    // Also pass through the function live values.
    out << "  " << input_csignal(fname, idx, i, "live") << " = "
        << output_csignal(fname, b.pred[i]->id, pred_sidx, "live") << ';'
        << endl;
  }
  if (entry) {
    n_pred++;
    out << "  " << input_signal(fname, idx, b.pred.size(), "valid")
        << " = _(" << fname << "_call, \"valid\");" << endl
        << "  _(" << fname << "_call, \"ready\") = "
        << input_signal(fname, idx, b.pred.size(), "ready") << ';' << endl
        << "  " << input_csignal(fname, idx, b.pred.size(), "live")
        << " = _(_(" << fname << "_call, \"contents\"), \"live\");" << endl;
  }

  // Instantiate arbiter and input buffer.
  out << "  BBArbiter(" << fname << "_bb" << idx << "_in_prebuf, "
      << fname << "_bb" << idx << "_arb_in);" << endl
      << "  BBInputBuf";
  if (b.cycle_breaker) out << '2';
  out << "(" << fname << "_bb" << idx << "_in, "
      << fname << "_bb" << idx <<  "_in_prebuf);" << endl;

  // Declare block's run signal
  out << "  node " << fname << "_bb" << idx << "_run;"
      << "  TAP(" << fname << "_bb" << idx << "_run);" << endl;
  
  for (unsigned i = 0; i < b.vals.size(); ++i) {
    gen_val(out, fname, idx, i, b, *b.vals[i]);
    if (b.vals[i]->t.type_vec.size() > 0) {
      out << "  tap(\"" << fname << '_' << b.vals[i]->id << "\", "
          << val_name(fname, idx, b, *b.vals[i]) << ");" << endl;
    }
  }

  // Connect our run signal.
  out << fname << "_bb" << idx << "_run = "
      << "_(" << fname << "_bb" << idx << "_in, \"valid\") && "
      << " _(" << fname << "_bb" << idx << "_in, \"ready\")";
  if (b.stall) out << " && !" << val_name(fname, idx, b, *b.stall);
  out << ';' << endl;
  
  // Connect preubuf outputs
  if (call && call->op == VAL_CALL) {
    out << "  " << output_signal(fname, idx, "valid") << " = _("
        << fname << "_call_" << call->id << "_ret, \"valid\")";
    if (b.stall) out << " && !" << val_name(fname, idx, b, *b.stall);
    out << ';' << endl
        << "  _(" << fname << "_call_" << call->id
        << "_ret, \"ready\") = " << output_signal(fname, idx, "ready")
        << ';' << endl
        << "  " << input_signal(fname, idx, "ready") << " = _("
        << fname << "_call_" << call->id << "_args, \"ready\")";
    if (b.stall) out << " && !" << val_name(fname, idx, b, *b.stall);
    print_spawn_readys(out, fname, b);
    out << ';' << endl;
    out << output_signal(fname, idx, "contents") << " = _(_("
        << fname << "_call_" << call->id
        << "_ret, \"contents\"), \"live\");" << endl;
  } else {
    out << "  " << output_signal(fname, idx, "valid") << " = "
        << input_signal(fname, idx, "valid");
    if (b.stall) out << " && !" << val_name(fname, idx, b, *b.stall);
    print_spawn_readys(out, fname, b);
    out << ';' << endl
        << "  " << input_signal(fname, idx, "ready") << " = "
        << output_signal(fname, idx, "ready");
    if (b.stall) out << " && !" << val_name(fname, idx, b, *b.stall);
    print_spawn_readys(out, fname, b);
    out << ';' << endl;
  }

  // Connections of live values to output.
  for (auto &x : b.live_out) {
    if (x == call) continue;

    std::ostringstream oss;
    oss << "val" << x->id;
    out << "  " << output_csignal(fname, idx, oss.str()) << " = "
        << val_name(fname, idx, b, *x) << ';' << endl;
  }

  // Connection of function live value to output.
  out << "  " << output_csignal(fname, idx, "live") << " = "
      << "_(_(" << fname << "_bb" << idx << "_in, \"contents\"), \"live\");"
      << endl;

  // Instantiate buffer/switch
  if (!call || call->op == VAL_SPAWN) {
    out << "  _(" << fname << "_bb" << idx << "_out_prebuf, \"contents\") = "
        << fname << "_bb" << idx << "_live;" << endl;
  } else if (call && call->op == VAL_CALL) {
    bool call_live(
      find(b.live_out.begin(), b.live_out.end(), call) != b.live_out.end()
    );

    if (call_live)
      out << "  _(_(" << fname << "_bb" << idx << "_out_prebuf, \"contents\"), \"val" << call->id << "\") = " << fname << '_' << call->id << ';' << endl;
  } 

  if (b.suc.size() == 1) {
    out << "  BBOutputBuf(" << fname << "_bb" << idx << "_out, " << fname
        << "_bb" << idx << "_out_prebuf);" << endl;
  } else if (b.suc.size() >= 2 && b.branch_pred) {
    out << "  BBOutputBuf(" << val_name(fname, idx, b, *b.branch_pred) << ", "
        << fname << "_bb" << idx << "_out, " << fname << "_bb" << idx
        << "_out_prebuf);" << endl;
  } else if (b.suc.size() >= 2) {
    out << "  BBOutputSpawnBuf(" << fname << "_bb" << idx << "_out, " << fname
        << "_bb" << idx << "_out_prebuf);" << endl;
  }

  out << "  hierarchy_exit();" << endl << endl;
}

static void pgen::gen_func_decls(std::ostream &out, std::string name, if_func &f) {
  using std::endl;
  
  // Typedef call/ret types
  out << "template <typename T> using " << name << "_ret_t = "
      << "flit<ag<STP(\"live\"), T, ag<STP(\"rval\"), "
      << type_chdl(f.rtype) << " > > >;" << endl;

  out << "template <typename T> using " << name << "_call_contents_t = "
      << "ag<STP(\"live\"), T";
  if (f.args.size() > 0) out << ',';
  print_arg_type(out, f.args);
  out << " >;" << endl;
  out << "template <typename T> using " << name << "_call_t = flit<" << name
      << "_call_contents_t<T> >;" << endl;

  // Prototype the function.
  out << "template <typename OPAQUE> void " << name
      << '(' << name << "_ret_t<OPAQUE> &" << name << "_ret, "
      << name << "_call_t<OPAQUE> &" << name << "_call);"
      << endl;
}

static void pgen::gen_func(std::ostream &out, std::string name, if_func &f) {
  using std::endl;
  
  out << "template <typename OPAQUE> void " << name
      << '(' << name << "_ret_t<OPAQUE> &" << name << "_ret, "
      << name << "_call_t<OPAQUE> &" << name << "_call) {"
      << endl;

  // count_args(f);
  
  for (auto &s : f.static_vars) {
    unsigned loads = s.second.load_count,
             stores = s.second.store_count;
    gen_static_var(out, s.second, name, loads, stores);
  }
  out << endl;

  for (unsigned i = 0; i < f.bbs.size(); ++i)
    gen_bb_decls(out, name, i, *f.bbs[i], i == 0);

  // Connect args to first basic block's input
  out << "  _(_(" << name << "_bb0_arb_in[0], \"contents\"), \"args\") = _("
      << name << "_call, \"contents\");" << endl;
  
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    gen_bb(out, name, i, *f.bbs[i], i == 0);
  }

  for (auto &s : f.static_vars)
    gen_static_var_bottom(out, s.second, name);

  out << '}' << endl;
}

void pgen::gen_prog(std::ostream &out, if_prog &p) {
  for (auto &f : p.functions) {
    gen_func_decls(out, f.first, f.second);
  }
  
  for (auto &f : p.functions) {
    gen_func(out, f.first, f.second);
  }
}
