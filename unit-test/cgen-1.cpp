#include <iostream>

#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"

using namespace bscotch;
using namespace std;

template <unsigned N> void to_vec_bool(vector<bool> &v, unsigned long x) {
  v.clear();
  for (unsigned i = 0; i < N; ++i)
    v.push_back(x & (1ul<<i));
}

type uN(int n) {
  type t;
  t.type_vec.push_back(TYPE_U);
  t.type_vec.push_back(n);

  return t;
}

type u32() {
  return uN(32);
}

type void_type() {
  type t;
  return t;
}

type bit_type() {
  type t;
  t.type_vec.push_back(TYPE_BIT);

  return t;
}

if_staticvar u32staticvar(string name, unsigned initial = 0) {
  if_staticvar s;

  s.name = name;
  to_vec_bool<32>(s.initial_val, initial);

  s.t = u32();

  return s;
}

bool is_store(if_op op) {
  return op == VAL_ST_GLOBAL || op == VAL_ST_STATIC ||
         op == VAL_ST_IDX || op == VAL_ST_IDX_STATIC;
}

void gen_static_var(ostream &out, if_staticvar &s, string fname, bool global) {
  out << "  STATIC_VAR(" << fname << ", " << s.name << ", " << type_chdl(s.t)
      << ", 0x" << to_hex(s.initial_val) << ");" << endl;
}

void gen_static_var_bottom
  (ostream &out, if_staticvar &s, string fname, bool global)
{
  out << "  STATIC_VAR_GEN(" << fname << ", " << s.name << ");" << endl;
}

void gen_val(ostream &out, string fname, int bbidx, int idx, if_val &v) {
  out << "  ";
  
  if (!is_store(v.op)) {
    out << type_chdl(v.t) << ' ' << fname << '_' << v.id << " = ";
  }
  
  if (v.op == VAL_CONST) {
    out << "Lit<" << v.t.size() << ">(0x" << to_hex(v.const_val) << ')';
  } else if (v.op == VAL_LD_STATIC) {
    out << "LD_STATIC(" << fname << ", " << v.static_arg->name << ')';
  } else if (v.op == VAL_ADD) {
    out << fname << '_' << v.args[0]->id << " + "
	<< fname << '_' << v.args[1]->id;
  } else if (v.op == VAL_ST_STATIC) {
    out << "ST_STATIC(" << fname << ", " << v.static_arg->name << ", "
        << fname << '_' << v.args[0]->id << ", "
        << fname << "_bb" << bbidx << "_run)";
  } else {
    out << "UNSUPPORTED_OP " << if_op_str[v.op];
  }

  out << ';' << endl;
}

void print_arg_type(ostream &out, vector<type> &v) {
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

void print_live_type(ostream &out, vector<if_val*> &v) {
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

string input_signal(string fname, int bbidx, int vidx, string signal) {
  ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_arb_in[" <<  vidx << "], \""
      << signal << "\")";
  return oss.str();
}

string input_signal(string fname, int bbidx, string signal) {
  ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_in, \"" << signal << "\")";
  return oss.str();
}

string output_signal(string fname, int bbidx, int vidx, string signal) {
  ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_out[" <<  vidx << "], \""
      << signal << "\")";
  return oss.str();
}

string output_signal(string fname, int bbidx, string signal) {
  ostringstream oss;
  oss << "_(" << fname << "_bb" << bbidx << "_out_prebuf, \"" << signal
      << "\")";
  return oss.str();
}

void gen_bb(ostream &out, string fname, int idx, if_bb &b, bool entry) {
  // Typedef input/output types
  out << "  typedef flit<";
  print_live_type(out, b.live_in);
  out << "> " << fname << "_bb" << idx << "_in_t;" << endl;
  
  out << "  typedef flit<";
  print_live_type(out, b.live_out);
  out << "> " << fname << "_bb" << idx << "_out_t;" << endl;
  
  // Declare input/output arrays
  out << "  " << fname << "_bb" << idx << "_in_t "
      << fname << "_bb" << idx << "_in;" << endl;

  int n_suc = b.suc.size();
  out << "  " << fname << "_bb" << idx << "_out_t " << fname << "_bb" << idx
      << "_out_prebuf;" << endl 
      << "  vec<" << n_suc << ", " << fname << "_bb" << idx << "_out_t> "
      << fname << "_bb" << idx << "_out;" << endl;

  int n_pred = b.pred.size();
  if (entry) n_pred++;
  out << "  vec<" << n_pred << ", " << fname << "_bb" << idx << "_in_t> "
      << fname << "_bb" << idx << "_arb_in;" << endl;
  
  // Set up arbiter inputs
  for (unsigned i = 0; i < b.pred.size(); ++i) {
    int pred_id(b.pred[i]->id), pred_sidx = -1;
    for (unsigned j = 0; j < b.pred[i]->suc.size(); ++j) {
      if (b.pred[i]->suc[j] == &b) {
	pred_sidx = j;
      }
    }

    if (pred_sidx == -1) abort();
    
    out << "  " << input_signal(fname, idx, i, "valid")
        << " = " << output_signal(fname, b.pred[i]->id, pred_sidx, "valid") << ';' << endl
        << "  " << output_signal(fname, b.pred[i]->id, pred_sidx, "ready") << " = "
        << input_signal(fname, idx, i, "ready") << ';' << endl;
  }
  if (entry) {
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
    gen_val(out, fname, idx, i, b.vals[i]);
  }

  // Connect preubuf outputs
  out << "  " << output_signal(fname, idx, "valid") << " = "
      << input_signal(fname, idx, "valid") << ';' << endl
      << "  " << input_signal(fname, idx, "ready") << " = "
      << output_signal(fname, idx, "ready") << ';' << endl;

  // Instantiate buffer/switch
  if (b.suc.size() == 1) {
    out << "  BBOutputBuf(" << fname << "_bb" << idx << "_out, " << fname << "_bb" << idx << "_out_prebuf);" << endl;
  } else {
    out << "  BBOutputBuf(" << fname << '_' << b.branch_pred->id << ", " << fname << "_bb" << idx << "_out, " << fname << "_bb" << idx << "_out_prebuf);" << endl;
  }
}

void gen_func(ostream &out, string name, if_func &f) {
  for (auto &s : f.static_vars)
    gen_static_var(out, s.second, name, false);

  // Typedef call/ret types
  out << "  typedef flit<";
  print_arg_type(out, f.args);
  out << " > " << name << "_call_t;" << endl;
  
  out << "  typedef flit<" << type_chdl(f.rtype) << " > "
      << name << "_ret_t;" << endl;

  out << "  " << name << "_call_t " << name << "_call;" << endl
      << "  " << name << "_ret_t " << name << "_ret;" << endl;
  
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    gen_bb(out, name, i, f.bbs[i], i == 0);
  }

  for (auto &s : f.static_vars)
    gen_static_var_bottom(out, s.second, name, false);
}

void gen_prog(ostream &out, if_prog &p) {
  for (auto &f : p.functions) {
    gen_func(out, f.first, f.second);
  }
}

void test_func(if_func &f) {
  // Test function:
  // static var x : u32, initial val 0
  //  bb0:
  //    %0 = const #0 (u32)
  //  bb1:
  //    %1 = phi %0, %11 (u32)
  //    %2 = const #0 (u5)
  //    %3 = %1[%2] (bit)
  //    br %3, bb2, bb3
  //  bb2:
  //    %4 = ld_static @x (u32)
  //    %5 = add %4, %1 (u32)
  //    %6 = st_static @x, %5 (void)
  //    br bb4
  //  bb3:
  //    %7 = ld_static @x (u32)
  //    %8 = sub %7, %1 (u32)
  //    %9 = st_static @x, %7 (void)
  //  bb4:
  //    %10 = const #1 (u32)
  //    %11 = add %1, %10 (u32)
  //    br bb1

  f.static_vars["x"] = u32staticvar("x", 0);
  
  f.rtype = void_type();

  f.bbs.resize(5);

  f.bbs[0].vals.resize(1);
  f.bbs[1].vals.resize(3);
  f.bbs[2].vals.resize(3);
  f.bbs[3].vals.resize(3);
  f.bbs[4].vals.resize(2);

  f.bbs[0].suc.push_back(&f.bbs[1]);
  f.bbs[1].pred.push_back(&f.bbs[0]);
  f.bbs[1].suc.push_back(&f.bbs[2]);
  f.bbs[2].pred.push_back(&f.bbs[1]);
  f.bbs[1].suc.push_back(&f.bbs[3]);
  f.bbs[3].pred.push_back(&f.bbs[1]);
  f.bbs[2].suc.push_back(&f.bbs[4]);
  f.bbs[4].pred.push_back(&f.bbs[2]);
  f.bbs[3].suc.push_back(&f.bbs[4]);
  f.bbs[4].pred.push_back(&f.bbs[3]);
  f.bbs[4].suc.push_back(&f.bbs[1]);
  f.bbs[1].pred.push_back(&f.bbs[4]);

  f.bbs[0].branch_pred = NULL;
  f.bbs[1].branch_pred = &f.bbs[1].vals[1];
  f.bbs[2].branch_pred = NULL;
  f.bbs[3].branch_pred = NULL;
  f.bbs[4].branch_pred = NULL;

  int id = 0;
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    f.bbs[i].id = i;
    for (unsigned j = 0; j < f.bbs[i].vals.size(); ++j) {
      f.bbs[i].vals[j].bb = &f.bbs[i];
      f.bbs[i].vals[j].id = id++;
    }
  }

  f.bbs[0].vals[0].t = u32();
  f.bbs[0].vals[0].op = VAL_CONST;
  to_vec_bool<32>(f.bbs[0].vals[0].const_val, 0);

  f.bbs[1].vals[0].t = u32();
  f.bbs[1].vals[0].op = VAL_PHI;
  f.bbs[1].vals[0].args.push_back(&f.bbs[0].vals[0]);
  f.bbs[1].vals[0].args.push_back(&f.bbs[4].vals[1]);
  
  f.bbs[1].vals[1].t = uN(5);
  f.bbs[1].vals[1].op = VAL_CONST;
  to_vec_bool<5>(f.bbs[1].vals[1].const_val, 0);
  
  f.bbs[1].vals[2].t = bit_type();
  f.bbs[1].vals[2].op = VAL_LD_IDX_STATIC;
  f.bbs[1].vals[2].args.push_back(&f.bbs[1].vals[0]);
  f.bbs[1].vals[2].args.push_back(&f.bbs[1].vals[1]);

  f.bbs[2].vals[0].t = u32();
  f.bbs[2].vals[0].op = VAL_LD_STATIC;
  f.bbs[2].vals[0].static_arg = &f.static_vars["x"];

  f.bbs[2].vals[1].t = u32();
  f.bbs[2].vals[1].op = VAL_ADD;
  f.bbs[2].vals[1].args.push_back(&f.bbs[2].vals[0]);
  f.bbs[2].vals[1].args.push_back(&f.bbs[1].vals[0]);

  f.bbs[2].vals[2].t = void_type();
  f.bbs[2].vals[2].op = VAL_ST_STATIC;
  f.bbs[2].vals[2].static_arg = &f.static_vars["x"];
  f.bbs[2].vals[2].args.push_back(&f.bbs[2].vals[1]);

  f.bbs[3].vals[0].t = u32();
  f.bbs[3].vals[0].op = VAL_LD_STATIC;
  f.bbs[3].vals[0].static_arg = &f.static_vars["x"];

  f.bbs[3].vals[1].t = u32();
  f.bbs[3].vals[1].op = VAL_SUB;
  f.bbs[3].vals[1].args.push_back(&f.bbs[3].vals[0]);
  f.bbs[3].vals[1].args.push_back(&f.bbs[1].vals[0]);

  f.bbs[3].vals[2].t = void_type();
  f.bbs[3].vals[2].op = VAL_ST_STATIC;
  f.bbs[3].vals[2].static_arg = &f.static_vars["x"];
  f.bbs[3].vals[2].args.push_back(&f.bbs[3].vals[1]);

  f.bbs[4].vals[0].t = u32();
  f.bbs[4].vals[0].op = VAL_CONST;
  to_vec_bool<32>(f.bbs[4].vals[0].const_val, 1);

  f.bbs[4].vals[1].t = void_type();
  f.bbs[4].vals[1].op = VAL_ADD;
  f.bbs[4].vals[1].args.push_back(&f.bbs[1].vals[0]);
  f.bbs[4].vals[1].args.push_back(&f.bbs[4].vals[0]);
}

void test_prog(if_prog &p) {
  // Test program:

  test_func(p.functions["main"]);
}

int main() {
  if_prog p;

  test_prog(p);

  print(cout, p);

  // gen_prog(cout, p);
  
  return 0;
}
