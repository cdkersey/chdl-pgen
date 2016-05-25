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

type u32() {
  type t;
  t.type_vec.push_back(TYPE_U);
  t.type_vec.push_back(32);

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

void gen_bb(ostream &out, string fname, int idx, if_bb &b) {
  for (unsigned i = 0; i < b.vals.size(); ++i) {
    gen_val(out, fname, idx, i, b.vals[i]);
  }
}

void gen_func(ostream &out, string name, if_func &f) {
  for (auto &s : f.static_vars) {
    gen_static_var(out, s.second, name, false);
  }
  
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    gen_bb(out, name, i, f.bbs[i]);
  }
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
  //    %5 = const #1
  //    %10 = ld_static @x
  //    %15 = add %10, %5
  //    %20 = st_static %15, @x

  f.static_vars["x"] = u32staticvar("x", 0);

  f.bbs.push_back(if_bb());

  f.bbs[0].vals.push_back(if_val());
  f.bbs[0].vals[0].t = u32();
  f.bbs[0].vals[0].op = VAL_CONST;
  f.bbs[0].vals[0].static_arg = NULL;
  f.bbs[0].vals[0].id = 0;
  to_vec_bool<32>(f.bbs[0].vals[0].const_val, 1);

  f.bbs[0].vals.push_back(if_val());
  f.bbs[0].vals[1].t = u32();
  f.bbs[0].vals[1].op = VAL_LD_STATIC;
  f.bbs[0].vals[1].id = 1;
  f.bbs[0].vals[1].static_arg = &f.static_vars["x"];
  
  f.bbs[0].vals.push_back(if_val());
  f.bbs[0].vals[2].t = u32();
  f.bbs[0].vals[2].op = VAL_ADD;
  f.bbs[0].vals[2].args.push_back(&f.bbs[0].vals[1]);
  f.bbs[0].vals[2].args.push_back(&f.bbs[0].vals[0]);
  f.bbs[0].vals[2].static_arg = NULL;
  f.bbs[0].vals[2].id = 2;

  f.bbs[0].vals.push_back(if_val());
  f.bbs[0].vals[3].op = VAL_ST_STATIC;
  f.bbs[0].vals[3].static_arg = &f.static_vars["x"];
  f.bbs[0].vals[3].args.push_back(&f.bbs[0].vals[2]);
  f.bbs[0].vals[3].id = 3;

  f.bbs[0].suc.push_back(&f.bbs[0]);
  f.bbs[0].branch_pred = NULL;
}

void test_prog(if_prog &p) {
  // Test program:

  test_func(p.functions["main"]);
}

int main() {
  if_prog p;

  test_prog(p);

  print(cout, p);

  gen_prog(cout, p);
  
  return 0;
}
