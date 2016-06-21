#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#include "type.h"
#include "if.h"
#include "cgen.h"

using namespace bscotch;

static bool is_store(if_op op) {
  return op == VAL_ST_GLOBAL || op == VAL_ST_STATIC ||
         op == VAL_ST_IDX || op == VAL_ST_IDX_STATIC;
}

static bool is_load(if_op op) {
  return op == VAL_LD_GLOBAL || op == VAL_LD_STATIC ||
         op == VAL_LD_IDX || op == VAL_LD_IDX_STATIC;
}

static void gen_static_var(
  std::ostream &out, if_staticvar &s, std::string fname,
  unsigned loads, unsigned stores, bool global)
{
  using std::endl;

  if (is_sram_array(s.t)) {
    out << "  STATIC_ARRAY(" << fname << ", " << s.name << ", "
        << type_chdl(element_type(s.t)) << ", " << array_len(s.t) << ", "
        << loads << ");" << endl;
  } else {
    out << "  STATIC_VAR(" << fname << ", " << s.name << ", " << type_chdl(s.t)
        << ", 0x" << to_hex(s.initial_val) << ", " << stores << ");" << endl;
  }
}

static void gen_static_var_bottom
  (std::ostream &out, if_staticvar &s, std::string fname, bool global)
{
  using std::endl;
  out << "  STATIC_VAR_GEN(" << fname << ", " << s.name << ");" << endl;
}

static bool is_binary(if_op o) {
  return (o == VAL_ADD) || (o == VAL_SUB) || (o == VAL_MUL) || (o == VAL_DIV) ||
         (o == VAL_AND) || (o == VAL_OR) || (o == VAL_XOR);
}

static bool is_unary(if_op o) {
   return (o == VAL_NEG) || (o == VAL_NOT);
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

  return " UNSUPPORTED OP ";
 }

static std::string val_name(std::string fname, int bbidx, if_bb &b, if_val &v)
{
  std::ostringstream oss;
  bool in_block = false;
  for (auto &y : b.vals)
    if (y.id == v.id && v.op != VAL_PHI) in_block = true;

  if (in_block)
    oss << fname << '_' << v.id;
  else
    oss << "_(_(" << fname << "_bb" << bbidx << "_in, \"contents\"), \"val"
        << v.id << "\")";
  return oss.str();
}

static std::string call_type(bool has_id, std::vector<if_val *> args) {
  using namespace std;

  if (!has_id && args.size() == 0) {
    return "chdl_void";
  }

  ostringstream oss;
  
  int arg_id = 0;
  for (auto &p : args) {
    oss << "ag<";

    if (has_id) {
      has_id = false;
      oss << "STP(\"id\"), ";
    } else {
      oss << "STP(\"arg" << arg_id++ << "\"), ";
    }

    oss << type_chdl(p->t);

    if (p != *args.rbegin()) oss << ", ";
  }

  for (auto &p : args)
    oss << " >";

  return oss.str();
}

static std::string ret_type(bool has_id, if_val *ret, type *id_t = NULL) {
  std::string r;

  if (has_id) {
    r = "ag<STP(\"id\"), " + type_chdl(*id_t) + ", ";
  }

  r += "ag<STP(\"rval\"), " + type_chdl(ret->t) + " >";

  if (has_id) r += " >";

  return r;
}

void bscotch::gen_val(std::ostream &out, std::string fname, int bbidx, int idx, if_bb &b, if_val &v)
{
  using std::endl;
  using std::string;
  using std::vector;
  using std::ostringstream;
  out << "  ";

  if (!is_store(v.op) && v.op != VAL_PHI && v.op != VAL_CALL) {
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
    } else {
      out << "Lit<" << v.t.size() << ">(0x" << to_hex(v.const_val) << ')';
    }
  } else if (v.op == VAL_LD_STATIC) {
    out << "LD_STATIC(" << fname << ", " << v.static_arg->name << ')';
  } else if (is_binary(v.op)) {
    out << aname[0] << op_str(v.op, v.args[0]->t, v.args[1]->t) << aname[1];
  } else if (is_unary(v.op)) {
    out << op_str(v.op, v.args[0]->t) << aname[0];
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

      string fieldname = v.args[0]->t.field_name[const_val(*v.args[1])];

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
      string fieldname = v.args[0]->t.field_name[const_val(*v.args[1])];

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
          << idx + len << ">()]" << " = " << aname[3] << ';';
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
  } else if (v.op == VAL_CALL) {
    ostringstream ctype_oss, rtype_oss;
    ctype_oss << v.func_arg << "_call_t<" << fname << "_bb" << bbidx
              << "_out_contents_t" << '>';
    rtype_oss << v.func_arg << "_ret_t<" << fname << "_bb" << bbidx
              << "_out_contents_t" << '>';
    
    string rtype = rtype_oss.str(),
           ctype = ctype_oss.str();

    out << rtype << ' ' << fname << "_call_" << v.id << "_ret;" << endl
        << "  " << ctype << ' ' << fname << "_call_" << v.id << "_args;" << endl
        << "  TAP(" << fname << "_call_" << v.id << "_ret);" << endl
        << "  TAP(" << fname << "_call_" << v.id << "_args);" << endl;

    out << "  _(" << fname << "_call_" << v.id << "_args, \"valid\") = _("
        << fname << "_bb" << bbidx << "_in, \"valid\");" << endl;
    out << "  _(" << fname << "_call_" << v.id << "_ret, \"ready\") = _("
        << fname << "_bb" << bbidx << "_out_prebuf, \"ready\");" << endl;

    for (unsigned i = 0; i < v.args.size(); ++i) {
      out << "  _(_(" << fname << "_call_" << v.id
          << "_args, \"contents\"), \"arg" << i << "\") = "
          << aname[i] << ';' << endl;
    }

    out << "_(_(" << fname << "_call_" << v.id << "_args, \"contents\"), \"live\") = " << fname << "_bb" << bbidx << "_live;" << endl;

    #if 0
    ostringstream lsname;
    lsname << fname << "_call_" << v.id << "_live";
      
    out << "  " << fname << "_bb" << bbidx << "_out_t " << lsname.str()
        << ';' << endl
        << "  _(" << lsname.str() << ", \"contents\") = LLRam(_(_(" << fname
        << "_call_" << v.id << "_ret, \"contents\"), \"id\"), Flatten(_("
        << fname << "_bb" << bbidx << "_out_prebuf, \"contents\")), _(_("
        << fname << "_call_" << v.id << "_args, \"contents\"), \"id\"), _("
        << fname << "_call_" << v.id << "_args, \"ready\") && _(" << fname
        << "_call_" << v.id << "_args, \"valid\"));" << endl
        << "  _(" << lsname.str() << ", \"ready\") = _(" << fname
        << "_call_" << v.id << "_args, \"ready\");" << endl
        << "  _(" << lsname.str() << ", \"valid\") = _(" << fname << "_call_"
        << v.id << "_ret, \"valid\");" << endl
        << "  TAP(" << lsname.str() << ");\n";
    #endif
    
    bool rval_live = false;
    for (auto &p : b.live_out)
      if (p == &v) rval_live = true;
      
    if (rval_live)
      out << "  _(_(_(" << fname << "_call_" << v.id
          << "_ret, \"contents\"), \"live\"), \"val" << v.id
          << "\") = _(_(" << fname << "_call_" << v.id
          << "_ret, \"contents\"), \"rval\");" << endl;    
    out << "  " << v.func_arg << '(' << fname << "_call_" << v.id << "_ret, "
        << fname << "_call_" << v.id << "_args);" << endl;

    out << "  " << type_chdl(v.t) << ' ' << fname << '_' << v.id << " = "
        << "_(_(" << fname << "_call_" << v.id
        << "_ret, \"contents\"), \"rval\");" << endl;
  } else {
    out << "UNSUPPORTED_OP " << if_op_str[v.op];
  }

  if (v.op != VAL_PHI && v.op != VAL_CALL) out << ';' << endl;
}

static void print_arg_type(std::ostream &out, std::vector<type> &v) {
  if (v.size() == 0) {
    out << "chdl_void";
  } else {
    for (unsigned i = 0; i < v.size(); ++i) {
      out << "ag<STP(\"" << "arg" << i << "\"), " << type_chdl(v[i]);
      if (i != v.size() - 1) out << ", ";
    }
    for (unsigned i = 0; i < v.size(); ++i)
      out << " >";
  }
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

void bscotch::gen_bb_decls(std::ostream &out, std::string fname, int idx, if_bb &b, bool entry) {
  using std::endl;

  out << "  // " << fname << " BB " << idx << " declarations" << endl;
  out << "  hierarchy_enter(\"" << fname << "_bb" << idx << "_decls\");" << endl;
  // Typedef input/output types
  out << "  typedef ";
  print_live_type(out, b.live_in);
  out << ' ' << fname << "_bb" << idx << "_in_contents_t;" << endl
      << "  typedef flit<" << fname << "_bb" << idx << "_in_contents_t> "
      << fname << "_bb" << idx << "_in_t;" << endl;

  out << "  typedef ";
  print_live_type(out, b.live_out);
  out << ' ' << fname << "_bb" << idx << "_out_contents_t;" << endl
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

static void get_val_map(std::map<int, int> &m, if_bb &a, if_bb &b) {
  // The only ones that are not 1-1 from the live_in/live_out sets are the
  // ones corresponding to phis. Scan the destination BB for phis, and insert
  // accordingly.
  for (auto &v : b.vals) {
    if (v.op == VAL_PHI) {
      int src = -1;
      for (auto &s : v.args) {
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
      }
      m[v.id] = src;
    }
  }

  // The non-phis are all just 1-1 copies.
  for (auto &x : b.live_in)
    if (!m.count(x->id)) m[x->id] = x->id;
}
 
void bscotch::gen_bb(std::ostream &out, std::string fname, int idx, if_bb &b, bool entry)
{
  using std::endl;
  using std::map;

  out << "  // " << fname << " BB " << idx << " body" << endl;
  out << "  hierarchy_enter(\"" << fname << "_bb" << idx << "\");" << endl;  
  int n_suc = b.suc.size(), n_pred = b.pred.size();
  
  // Set up arbiter inputs
  for (unsigned i = 0; i < b.pred.size(); ++i) {
    map<int, int> vmap;
    get_val_map(vmap, *b.pred[i], b);

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
  }
  if (entry) {
    n_pred++;
    out << "  " << input_signal(fname, idx, b.pred.size(), "valid")
        << " = _(" << fname << "_call, \"valid\");" << endl
        << "  _(" << fname << "_call, \"ready\") = "
        << input_signal(fname, idx, b.pred.size(), "ready") << ';' << endl;
  }
  
  // Instantiate arbiter and input buffer.
  out << "  BBArbiter(" << fname << "_bb" << idx << "_in_prebuf, "
      << fname << "_bb" << idx << "_arb_in);" << endl
      << "  BBInputBuf";
  if (b.cycle_breaker) out << '2';
  out << "(" << fname << "_bb" << idx << "_in, "
      << fname << "_bb" << idx <<  "_in_prebuf);" << endl;

  // Create block's run signal
  out << "  node " << fname << "_bb" << idx << "_run(_(" << fname << "_bb"
      << idx << "_in, \"valid\") && "
      << output_signal(fname, idx, "ready") << ");" << endl
      << "  TAP(" << fname << "_bb" << idx << "_run);" << endl;
  
  for (unsigned i = 0; i < b.vals.size(); ++i) {
    gen_val(out, fname, idx, i, b, b.vals[i]);
    if (b.vals[i].t.type_vec.size() > 0) {
      out << "  tap(\"" << fname << '_' << b.vals[i].id << "\", "
          << val_name(fname, idx, b, b.vals[i]) << ");" << endl;
    }
  }

  // Connect preubuf outputs
  if (b.vals.rbegin()->op == VAL_CALL) {
    out << "  " << output_signal(fname, idx, "valid") << " = _("
        << fname << "_call_" << b.vals.rbegin()->id << "_ret, \"valid\");"
        << endl
        << "  _(" << fname << "_call_" << b.vals.rbegin()->id
        << "_ret, \"ready\") = " << output_signal(fname, idx, "ready")
        << ';' << endl
        << "  " << input_signal(fname, idx, "ready") << " = _("
        << fname << "_call_" << b.vals.rbegin()->id << "_args, \"ready\");"
        << endl
        << output_signal(fname, idx, "contents") << " = _(_("
        << fname << "_call_" << b.vals.rbegin()->id
        << "_ret, \"contents\"), \"live\");" << endl;
  } else {
    out << "  " << output_signal(fname, idx, "valid") << " = "
        << input_signal(fname, idx, "valid") << ';' << endl
        << "  " << input_signal(fname, idx, "ready") << " = "
        << output_signal(fname, idx, "ready") << ';' << endl;
  }

  // Connections of live values to output.
  for (auto &x : b.live_out) {
    std::ostringstream oss;
    oss << "val" << x->id;
    out << "  " << output_csignal(fname, idx, oss.str()) << " = "
        << val_name(fname, idx, b, *x) << ';' << endl;
  }

  // Instantiate buffer/switch
  if (b.vals.rbegin()->op != VAL_CALL) {
    out << "  _(" << fname << "_bb" << idx << "_out_prebuf, \"contents\") = "
        << fname << "_bb" << idx << "_live;" << endl;
  }
  
  if (b.suc.size() == 1) {
    out << "  BBOutputBuf(" << fname << "_bb" << idx << "_out, " << fname
        << "_bb" << idx << "_out_prebuf);" << endl;
  } else {
    out << "  BBOutputBuf(" << val_name(fname, idx, b, *b.branch_pred) << ", "
        << fname << "_bb" << idx << "_out, " << fname << "_bb" << idx
        << "_out_prebuf);" << endl;
  }

  out << "  hierarchy_exit();" << endl << endl;
}

static void live_in_phi_adj(if_bb &b) {
  for (auto &v : b.vals) {
    if (v.op == VAL_PHI) {
      for (auto &a : v.args)
	b.live_in.erase(find(b.live_in.begin(), b.live_in.end(), a));
      b.live_in.push_back(&v);
    }
  }
}

unsigned count_loads(if_func &f, if_staticvar &s) {
  unsigned count = 0;

  for (auto &b : f.bbs)
    for (auto &v : b.vals)
      if (v.static_arg == &s)
	if (is_load(v.op))
	  v.static_access_id = count++;

  return count;
}

unsigned count_stores(if_func &f, if_staticvar &s) {
  unsigned count = 0;

  for (auto &b : f.bbs)
    for (auto &v : b.vals)
      if (v.static_arg == &s)
	if (is_store(v.op))
	  v.static_access_id = count++;

  return count;
}

void bscotch::gen_func(std::ostream &out, std::string name, if_func &f) {
  using std::endl;
  for (auto &s : f.static_vars) {
    unsigned loads = count_loads(f, s.second),
             stores = count_stores(f, s.second);
    gen_static_var(out, s.second, name, loads, stores, false);
  }

  out << endl;
  
  // Typedef call/ret types
  out << "  typedef flit<";
  print_arg_type(out, f.args);
  out << " > " << name << "_call_t;" << endl;
  
  out << "  typedef flit<" << type_chdl(f.rtype) << " > "
      << name << "_ret_t;" << endl;

  out << "  " << name << "_call_t " << name << "_call;" << endl
      << "  " << name << "_ret_t " << name << "_ret;" << endl << endl;

  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    live_in_phi_adj(f.bbs[i]);
    gen_bb_decls(out, name, i, f.bbs[i], i == 0);
  }
  
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    gen_bb(out, name, i, f.bbs[i], i == 0);
  }

  for (auto &s : f.static_vars)
    gen_static_var_bottom(out, s.second, name, false);
}

void bscotch::gen_prog(std::ostream &out, if_prog &p) {
  for (auto &f : p.functions) {
    gen_func(out, f.first, f.second);
  }
}

