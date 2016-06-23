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

void test_bmain(if_func &f) {
  // Test function:
  //  bb0:
  //    %0 = const #0 (u32)
  //  bb1:
  //    %1 = phi %0, %12 (u32)
  //    %2 = phi %0, %6 (u32)
  //    %3 = const #1103515245 (u32)
  //    %4 = const #12345 (u32)
  //    %5 = mul %2, %3
  //    %6 = add %4, %5
  //    %7 = const #10 (u32)
  //    %8 = xor %1, %7 (u32)
  //    %9 = or_reduce %8 (bit)
  //    %10 = not %9 (bit)
  //    %11 = const #1 (u32)
  //    %12 = add %11, %1
  //    %13 = spawn "func", %6
  //    br %19, bb1, bb2
  //  bb2:
  //    %9 = ret %1 (void)

  f.rtype = u32();

  f.bbs.resize(3);

  f.bbs[0].vals.resize(1);
  f.bbs[1].vals.resize(13);
  f.bbs[2].vals.resize(1);

  f.bbs[0].suc.push_back(&f.bbs[1]);
  f.bbs[1].suc.push_back(&f.bbs[1]);
  f.bbs[1].suc.push_back(&f.bbs[2]);
  f.bbs[1].pred.push_back(&f.bbs[0]);
  f.bbs[1].pred.push_back(&f.bbs[1]);
  f.bbs[2].pred.push_back(&f.bbs[1]);

  f.bbs[0].branch_pred = NULL;
  f.bbs[1].branch_pred = &f.bbs[1].vals[9];
  f.bbs[2].branch_pred = NULL;

  f.bbs[0].live_out.push_back(&f.bbs[0].vals[0]);
  f.bbs[1].live_in.push_back(&f.bbs[0].vals[0]);
  f.bbs[1].live_in.push_back(&f.bbs[1].vals[5]);
  f.bbs[1].live_in.push_back(&f.bbs[1].vals[11]);
  f.bbs[1].live_out.push_back(&f.bbs[1].vals[0]);
  f.bbs[1].live_out.push_back(&f.bbs[1].vals[5]);
  f.bbs[1].live_out.push_back(&f.bbs[1].vals[11]);
  f.bbs[2].live_in.push_back(&f.bbs[1].vals[0]);

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
  f.bbs[1].vals[0].args.push_back(&f.bbs[1].vals[11]);

  f.bbs[1].vals[1].t = u32();
  f.bbs[1].vals[1].op = VAL_PHI;
  f.bbs[1].vals[1].args.push_back(&f.bbs[0].vals[0]);
  f.bbs[1].vals[1].args.push_back(&f.bbs[1].vals[5]);
  
  f.bbs[1].vals[2].t = u32();
  f.bbs[1].vals[2].op = VAL_CONST;
  to_vec_bool<32>(f.bbs[1].vals[2].const_val, 1103515245);

  f.bbs[1].vals[3].t = u32();
  f.bbs[1].vals[3].op = VAL_CONST;
  to_vec_bool<32>(f.bbs[1].vals[3].const_val, 12345);

  f.bbs[1].vals[4].t = u32();
  f.bbs[1].vals[4].op = VAL_MUL;
  f.bbs[1].vals[4].args.push_back(&f.bbs[1].vals[1]);
  f.bbs[1].vals[4].args.push_back(&f.bbs[1].vals[2]);

  f.bbs[1].vals[5].t = u32();
  f.bbs[1].vals[5].op = VAL_ADD;
  f.bbs[1].vals[5].args.push_back(&f.bbs[1].vals[3]);
  f.bbs[1].vals[5].args.push_back(&f.bbs[1].vals[4]);

  f.bbs[1].vals[6].t = u32();
  f.bbs[1].vals[6].op = VAL_CONST;
  to_vec_bool<32>(f.bbs[1].vals[6].const_val, 10);
  
  f.bbs[1].vals[7].t = u32();
  f.bbs[1].vals[7].op = VAL_XOR;
  f.bbs[1].vals[7].args.push_back(&f.bbs[1].vals[0]);
  f.bbs[1].vals[7].args.push_back(&f.bbs[1].vals[6]);

  f.bbs[1].vals[8].t = bit_type();
  f.bbs[1].vals[8].op = VAL_OR_REDUCE;
  f.bbs[1].vals[8].args.push_back(&f.bbs[1].vals[7]);
  
  f.bbs[1].vals[9].t = bit_type();
  f.bbs[1].vals[9].op = VAL_NOT;
  f.bbs[1].vals[9].args.push_back(&f.bbs[1].vals[8]);

  f.bbs[1].vals[10].t = u32();
  f.bbs[1].vals[10].op = VAL_CONST;
  to_vec_bool<5>(f.bbs[1].vals[10].const_val, 1);

  f.bbs[1].vals[11].t = u32();
  f.bbs[1].vals[11].op = VAL_ADD;
  f.bbs[1].vals[11].args.push_back(&f.bbs[1].vals[0]);
  f.bbs[1].vals[11].args.push_back(&f.bbs[1].vals[10]);

  f.bbs[1].vals[12].t = void_type();
  f.bbs[1].vals[12].op = VAL_SPAWN;
  f.bbs[1].vals[12].func_arg = "func";
  f.bbs[1].vals[12].args.push_back(&f.bbs[1].vals[5]);

  f.bbs[2].vals[0].t = void_type();
  f.bbs[2].vals[0].op = VAL_RET;
  f.bbs[2].vals[0].args.push_back(&f.bbs[1].vals[0]);
}

void test_func(if_func &f) {
  // Test called function:
  // bb0:
  //  %0 = arg (u32)
  //  %1 = const #5 (u32)
  //  %2 = xor %0, %1 (u32)
  //  %3 = or_reduce %2 (bit)
  //  %4 = not %3 (bit)
  //  %5 = ret %4 (void)
  f.rtype = bit_type();
  f.args.push_back(u32());
  
  f.bbs.resize(1);
  f.bbs[0].vals.resize(6);
  f.bbs[0].branch_pred = NULL;
  f.bbs[0].id = 0;

  for (unsigned i = 0; i < f.bbs[0].vals.size(); ++i) {
    f.bbs[0].vals[i].id = i;
    f.bbs[0].vals[i].bb = &f.bbs[0];
  }

  f.bbs[0].vals[0].t = u32();
  f.bbs[0].vals[0].op = VAL_ARG;

  f.bbs[0].vals[1].t = u32();
  f.bbs[0].vals[1].op = VAL_CONST;
  to_vec_bool<32>(f.bbs[0].vals[1].const_val, 5);

  f.bbs[0].vals[2].t = u32();
  f.bbs[0].vals[2].op = VAL_XOR;
  f.bbs[0].vals[2].args.push_back(&f.bbs[0].vals[0]);
  f.bbs[0].vals[2].args.push_back(&f.bbs[0].vals[1]);

  f.bbs[0].vals[3].t = bit_type();
  f.bbs[0].vals[3].op = VAL_OR_REDUCE;
  f.bbs[0].vals[3].args.push_back(&f.bbs[0].vals[2]);

  f.bbs[0].vals[4].t = bit_type();
  f.bbs[0].vals[4].op = VAL_NOT;
  f.bbs[0].vals[4].args.push_back(&f.bbs[0].vals[3]);

  f.bbs[0].vals[5].t = void_type();
  f.bbs[0].vals[5].op = VAL_RET;
  f.bbs[0].vals[5].args.push_back(&f.bbs[0].vals[4]);
}

void test_prog(if_prog &p) {
  // Test program:
  test_bmain(p.functions["bmain"]);
  break_cycles(p.functions["bmain"]);

  test_func(p.functions["func"]);
  break_cycles(p.functions["func"]);
}

int main() {
  if_prog p;

  test_prog(p);

  // print(cout, p);

  gen_prog(cout, p);
  
  return 0;
}
