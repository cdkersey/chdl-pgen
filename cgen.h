#ifndef BSCOTCH_CGEN_H
#define BSCOTCH_CGEN_H

#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#include "type.h"
#include "if.h"

namespace bscotch {
bool is_store(if_op op) {
  return op == VAL_ST_GLOBAL || op == VAL_ST_STATIC ||
         op == VAL_ST_IDX || op == VAL_ST_IDX_STATIC;
}

void gen_static_var(std::ostream &out, if_staticvar &s, std::string fname, bool global) {
  using std::endl;
  out << "  STATIC_VAR(" << fname << ", " << s.name << ", " << type_chdl(s.t)
      << ", 0x" << to_hex(s.initial_val) << ");" << endl;
}

void gen_static_var_bottom
  (std::ostream &out, if_staticvar &s, std::string fname, bool global)
{
  using std::endl;
  out << "  STATIC_VAR_GEN(" << fname << ", " << s.name << ");" << endl;
}

bool is_al(if_op o) {
  return (o == VAL_ADD) || (o == VAL_SUB) ||
         (o == VAL_MUL) || (o == VAL_DIV) ||
         (o == VAL_AND) || (o == VAL_OR) ||
         (o == VAL_XOR);
}

bool type_is_bit(const type &t) {
  return t.type_vec.size() == 1 && t.type_vec[0] == TYPE_BIT;
}

std::string op_str(if_op o, const type &t, const type &u) {
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

std::string val_name(std::string fname, int bbidx, if_bb &b, if_val &v)
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
 
void gen_val(std::ostream &out, std::string fname, int bbidx, int idx, if_bb &b, if_val &v) {
  using std::endl;
  using std::string;
  using std::vector;
  out << "  ";
  
  if (!is_store(v.op) && v.op != VAL_PHI) {
    out << type_chdl(v.t) << ' ' << fname << '_' << v.id << " = ";
  }

  vector<string> aname;
  for (auto &x : v.args) {
    aname.push_back(val_name(fname, bbidx, b, *x));
  }
  
  if (v.op == VAL_CONST) {
    out << "Lit<" << v.t.size() << ">(0x" << to_hex(v.const_val) << ')';
  } else if (v.op == VAL_LD_STATIC) {
    out << "LD_STATIC(" << fname << ", " << v.static_arg->name << ')';
  } else if (is_al(v.op)) {
    out << aname[0] << op_str(v.op, v.args[0]->t, v.args[1]->t) << aname[1];
  } else if (v.op == VAL_ST_STATIC) {
    out << "ST_STATIC(" << fname << ", " << v.static_arg->name << ", "
        << aname[0] << ", "
        << fname << "_bb" << bbidx << "_run)";
  } else if (v.op == VAL_PHI) {
    // Do nothing; phis are handled entirely in the inputs to the block.
  } else if (v.op == VAL_LD_IDX_STATIC) {
    out << "Mux(" << aname[1] << ", " << aname[0] << ')';
  } else {
    out << "UNSUPPORTED_OP " << if_op_str[v.op];
  }

  if (v.op != VAL_PHI) out << ';' << endl;
}

void print_arg_type(std::ostream &out, std::vector<type> &v) {
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

void print_live_type(std::ostream &out, std::vector<if_val*> &v) {
  if (v.size() == 0) {
    out << "chdl_void";
  } else {
    for (unsigned i = 0; i < v.size(); ++i) {
      out << "ag<STP(\"" << "val" << v[i]->id << "\"), " << type_chdl(v[i]->t);
      if (i != v.size() - 1) out << ", ";
    }
    for (unsigned i = 0; i < v.size(); ++i)
      out << " >";
  }
}

std::string input_signal(std::string fname, int bbidx, int vidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_arb_in[" <<  vidx << "], \""
      << signal << "\")";
  return oss.str();
}

 std::string input_csignal(std::string fname, int bbidx, int vidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(_(" << fname << "_bb" << bbidx << "_arb_in[" <<  vidx << "], \"contents\"), \""
      << signal << "\")";
  return oss.str();
}

std::string input_signal(std::string fname, int bbidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_in, \"" << signal << "\")";
  return oss.str();
}

std::string output_signal(std::string fname, int bbidx, int vidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_out[" <<  vidx << "], \""
      << signal << "\")";
  return oss.str();
}

std::string output_csignal(std::string fname, int bbidx, int vidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(_(" << fname << "_bb" << bbidx << "_out[" <<  vidx
      << "], \"contents\"), \"" << signal << "\")";
  return oss.str();
}

 std::string output_csignal(std::string fname, int bbidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(_(" << fname << "_bb" << bbidx << "_out_prebuf, \"contents\"), \""
      << signal << "\")";
  return oss.str();
}

std::string output_signal(std::string fname, int bbidx, std::string signal) {
  std::ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_out_prebuf, \"" << signal
      << "\")";
  return oss.str();
}

void gen_bb_decls(std::ostream &out, std::string fname, int idx, if_bb &b, bool entry) {
  using std::endl;

  out << "  // " << fname << " BB " << idx << " declarations" << endl;
  // Typedef input/output types
  out << "  typedef flit<";
  print_live_type(out, b.live_in);
  out << " > " << fname << "_bb" << idx << "_in_t;" << endl;
  
  out << "  typedef flit<";
  print_live_type(out, b.live_out);
  out << " > " << fname << "_bb" << idx << "_out_t;" << endl;
  
  // Declare input/output arrays
  out << "  " << fname << "_bb" << idx << "_in_t "
      << fname << "_bb" << idx << "_in;" << endl;
  out << "TAP(" << fname << "_bb" << idx << "_in);" << endl;

  int n_suc = b.suc.size();
  out << "  " << fname << "_bb" << idx << "_out_t " << fname << "_bb" << idx
      << "_out_prebuf;" << endl 
      << "  vec<" << n_suc << ", " << fname << "_bb" << idx << "_out_t> "
      << fname << "_bb" << idx << "_out;" << endl;
  out << "TAP(" << fname << "_bb" << idx << "_out_prebuf);" << endl;

  int n_pred = b.pred.size();
  if (entry) n_pred++;
  out << "  vec<" << n_pred << ", " << fname << "_bb" << idx << "_in_t> "
      << fname << "_bb" << idx << "_arb_in;" << endl;

  out << endl;
}

void get_val_map(std::map<int, int> &m, if_bb &a, if_bb &b) {
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
 
void gen_bb(std::ostream &out, std::string fname, int idx, if_bb &b, bool entry)
{
  using std::endl;
  using std::map;

  out << "  // " << fname << " BB " << idx << " body" << endl;
  
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
  
  // Instantiate arbiter
  out << "  Arbiter(" << fname << "_bb" << idx << "_in, ArbRR<" << n_pred
      << ">, " << fname << "_bb" << idx << "_arb_in);" << endl;

  // Create block's run signal
  out << "  node " << fname << "_bb" << idx << "_run(_(" << fname << "_bb"
      << idx << "_in, \"valid\") && "
      << output_signal(fname, idx, "ready") << ");" << endl;
  
  for (unsigned i = 0; i < b.vals.size(); ++i) {
    gen_val(out, fname, idx, i, b, b.vals[i]);
  }

  // Connect preubuf outputs
  out << "  " << output_signal(fname, idx, "valid") << " = "
      << input_signal(fname, idx, "valid") << ';' << endl
      << "  " << input_signal(fname, idx, "ready") << " = "
      << output_signal(fname, idx, "ready") << ';' << endl;

  for (auto &x : b.live_out) {
    std::ostringstream oss;
    oss << "val" << x->id;
    out << "  " << output_csignal(fname, idx, oss.str()) << " = "
        << val_name(fname, idx, b, *x) << ';' << endl;
  }

  // Instantiate buffer/switch
  if (b.suc.size() == 1) {
    out << "  BBOutputBuf(" << fname << "_bb" << idx << "_out, " << fname << "_bb" << idx << "_out_prebuf);" << endl;
  } else {
    out << "  BBOutputBuf(" << fname << '_' << b.branch_pred->id << ", " << fname << "_bb" << idx << "_out, " << fname << "_bb" << idx << "_out_prebuf);" << endl;
  }

  out << endl;
}

void live_in_phi_adj(if_bb &b) {
  for (auto &v : b.vals) {
    if (v.op == VAL_PHI) {
      for (auto &a : v.args)
	b.live_in.erase(find(b.live_in.begin(), b.live_in.end(), a));
      b.live_in.push_back(&v);
    }
  }
}

void gen_func(std::ostream &out, std::string name, if_func &f) {
  using std::endl;
  for (auto &s : f.static_vars)
    gen_static_var(out, s.second, name, false);

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

void gen_prog(std::ostream &out, if_prog &p) {
  for (auto &f : p.functions) {
    gen_func(out, f.first, f.second);
  }
}

};

#endif
