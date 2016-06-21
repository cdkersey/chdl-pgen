#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include "../type.h"
#include "../if.h"
#include "../cgen.h"
#include "../break_cycles.h"  

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

type arr32(unsigned len) {
  type t = u32();

  t.type_vec.push_back(TYPE_ARRAY);
  t.type_vec.push_back(len);

  return t;
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

if_staticvar u32arraystaticvar(string name, unsigned len) {
  if_staticvar s;

  s.name = name;

  s.t = arr32(len);

  return s;
}

void test_func(if_func &f) {
  // Test function:
  //  bb0:
  //    %0 = const #0 (u32)
  //  bb1:
  //    %1 = phi %0, %7 (u32)
  //    %2 = const #0 (u5)
  //    %3 = const #5 (u5)
  //    %4 = %1[%2,%3] (u5)
  //    %5 = call func, %1
  //    br bb2
  //  bb2:
  //    %6 = const #1 (u32)
  //    %7 = add %1, %6
  //    br bb1

  f.rtype = void_type();

  f.bbs.resize(3);

  f.bbs[0].vals.resize(1);
  f.bbs[1].vals.resize(5);
  f.bbs[2].vals.resize(2);

  f.bbs[0].suc.push_back(&f.bbs[1]);
  f.bbs[1].suc.push_back(&f.bbs[2]);
  f.bbs[2].suc.push_back(&f.bbs[1]);
  f.bbs[1].pred.push_back(&f.bbs[0]);
  f.bbs[1].pred.push_back(&f.bbs[2]);
  f.bbs[2].pred.push_back(&f.bbs[1]);

  f.bbs[0].branch_pred = NULL;
  f.bbs[1].branch_pred = NULL;

  f.bbs[0].live_out.push_back(&f.bbs[0].vals[0]);
  f.bbs[1].live_in.push_back(&f.bbs[0].vals[0]);
  f.bbs[1].live_in.push_back(&f.bbs[2].vals[1]);
  f.bbs[1].live_out.push_back(&f.bbs[1].vals[0]);
  f.bbs[1].live_out.push_back(&f.bbs[1].vals[3]);
  f.bbs[1].live_out.push_back(&f.bbs[1].vals[4]);
  f.bbs[2].live_in.push_back(&f.bbs[1].vals[0]);
  f.bbs[2].live_in.push_back(&f.bbs[1].vals[3]);
  f.bbs[2].live_in.push_back(&f.bbs[1].vals[4]);
  f.bbs[2].live_out.push_back(&f.bbs[2].vals[1]);
  
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
  f.bbs[1].vals[0].args.push_back(&f.bbs[2].vals[1]);
  
  f.bbs[1].vals[1].t = uN(5);
  f.bbs[1].vals[1].op = VAL_CONST;
  to_vec_bool<5>(f.bbs[1].vals[1].const_val, 0);

  f.bbs[1].vals[2].t = uN(5);
  f.bbs[1].vals[2].op = VAL_CONST;
  to_vec_bool<5>(f.bbs[1].vals[2].const_val, 5);
  
  f.bbs[1].vals[3].t = uN(5);
  f.bbs[1].vals[3].op = VAL_LD_IDX;
  f.bbs[1].vals[3].args.push_back(&f.bbs[1].vals[0]);
  f.bbs[1].vals[3].args.push_back(&f.bbs[1].vals[1]);
  f.bbs[1].vals[3].args.push_back(&f.bbs[1].vals[2]);

  f.bbs[1].vals[4].t = u32();
  f.bbs[1].vals[4].op = VAL_CALL;
  f.bbs[1].vals[4].func_arg = "func";
  f.bbs[1].vals[4].args.push_back(&f.bbs[1].vals[0]);

  f.bbs[2].vals[0].t = u32();
  f.bbs[2].vals[0].op = VAL_CONST;
  to_vec_bool<32>(f.bbs[2].vals[0].const_val, 1);

  f.bbs[2].vals[1].t = u32();
  f.bbs[2].vals[1].op = VAL_ADD;
  f.bbs[2].vals[1].args.push_back(&f.bbs[1].vals[0]);
  f.bbs[2].vals[1].args.push_back(&f.bbs[2].vals[0]);
}

void test_prog(if_prog &p) {
  // Test program:
  test_func(p.functions["main"]);
  break_cycles(p.functions["main"]);
}

int main() {
  if_prog p;

  test_prog(p);

  // print(cout, p);

  gen_prog(cout, p);
  
  return 0;
}
