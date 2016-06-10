#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include "../type.h"
#include "../if.h"
#include "../cgen.h"

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

void test_func(if_func &f) {
  // Test function:
  // static var x : u32, initial val 0
  //  bb0:
  //    %0 = const #0 (u32)
  //  bb1:
  //    %1 = phi %0, %11 (u32)
  //    %2 = const #0 (u5)
  //    %3 = %1[%2] (bit)
  //    %4 = ld_static @x (u32)
  //    %5 = add %4, %1 (u32)
  //    %6 = sub %4, %1 (u32)
  //    %3 ? %7 = st_static @x, %5 (void)
  //    %8 = not %3 (bit)
  //    %8 ? %9 = st_static @x, %6 (void)
  //    %10 = const #1 (u32)
  //    %11 = add %1, %10 (u32)
  //    br bb1

  f.static_vars["x"] = u32staticvar("x", 0);
  
  f.rtype = void_type();

  f.bbs.resize(2);

  f.bbs[0].vals.resize(1);
  f.bbs[1].vals.resize(11);

  f.bbs[0].suc.push_back(&f.bbs[1]);
  f.bbs[1].suc.push_back(&f.bbs[1]);
  f.bbs[1].pred.push_back(&f.bbs[0]);
  f.bbs[1].pred.push_back(&f.bbs[1]);

  f.bbs[0].branch_pred = NULL;
  f.bbs[1].branch_pred = NULL;

  f.bbs[0].live_out.push_back(&f.bbs[0].vals[0]);
  f.bbs[1].live_in.push_back(&f.bbs[0].vals[0]);
  f.bbs[1].live_in.push_back(&f.bbs[1].vals[10]);
  f.bbs[1].live_out.push_back(&f.bbs[1].vals[10]);

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
  f.bbs[1].vals[0].args.push_back(&f.bbs[1].vals[10]);
  
  f.bbs[1].vals[1].t = uN(5);
  f.bbs[1].vals[1].op = VAL_CONST;
  to_vec_bool<5>(f.bbs[1].vals[1].const_val, 0);
  
  f.bbs[1].vals[2].t = bit_type();
  f.bbs[1].vals[2].op = VAL_LD_IDX;
  f.bbs[1].vals[2].args.push_back(&f.bbs[1].vals[0]);
  f.bbs[1].vals[2].args.push_back(&f.bbs[1].vals[1]);

  f.bbs[1].vals[3].t = u32();
  f.bbs[1].vals[3].op = VAL_LD_STATIC;
  f.bbs[1].vals[3].static_arg = &f.static_vars["x"];
  
  f.bbs[1].vals[4].t = u32();
  f.bbs[1].vals[4].op = VAL_ADD;
  f.bbs[1].vals[4].args.push_back(&f.bbs[1].vals[3]);
  f.bbs[1].vals[4].args.push_back(&f.bbs[1].vals[0]);

  f.bbs[1].vals[5].t = u32();
  f.bbs[1].vals[5].op = VAL_SUB;
  f.bbs[1].vals[5].args.push_back(&f.bbs[1].vals[3]);
  f.bbs[1].vals[5].args.push_back(&f.bbs[1].vals[0]);

  f.bbs[1].vals[6].pred = &f.bbs[1].vals[2];
  f.bbs[1].vals[6].t = void_type();
  f.bbs[1].vals[6].op = VAL_ST_STATIC;
  f.bbs[1].vals[6].static_arg = &f.static_vars["x"];
  f.bbs[1].vals[6].args.push_back(&f.bbs[1].vals[4]);

  f.bbs[1].vals[7].t = bit_type();
  f.bbs[1].vals[7].op = VAL_NOT;
  f.bbs[1].vals[7].args.push_back(&f.bbs[1].vals[2]);

  f.bbs[1].vals[8].pred = &f.bbs[1].vals[7];
  f.bbs[1].vals[8].t = void_type();
  f.bbs[1].vals[8].op = VAL_ST_STATIC;
  f.bbs[1].vals[8].static_arg = &f.static_vars["x"];
  f.bbs[1].vals[8].args.push_back(&f.bbs[1].vals[5]);

  f.bbs[1].vals[9].t = u32();
  f.bbs[1].vals[9].op = VAL_CONST;
  to_vec_bool<32>(f.bbs[1].vals[9].const_val, 1);

  f.bbs[1].vals[10].t = u32();
  f.bbs[1].vals[10].op = VAL_ADD;
  f.bbs[1].vals[10].args.push_back(&f.bbs[1].vals[0]);
  f.bbs[1].vals[10].args.push_back(&f.bbs[1].vals[9]);
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