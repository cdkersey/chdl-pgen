#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include "../type.h"
#include "../if.h"
#include "../cgen.h"
#include "../cgen-cpp.h"
#include "../break_cycles.h"

using namespace bscotch;
using namespace std;

template <unsigned N> void to_vec_bool(vector<bool> &v, unsigned long x) {
  v.clear();
  for (unsigned i = 0; i < N; ++i)
    v.push_back(x & (1ul<<i));
}

type uN(int n) { return u(n); }
type u32() { return u(32); }

type bit_type() {
  type t;
  t.type_vec.push_back(TYPE_BIT);

  return t;
}

if_staticvar u3bcastvar(string name) {
  if_staticvar s;

  s.name = name;
  s.t = uN(3);
  s.broadcast = true;

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
  //    %7 = const #100 (u32)
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

  for (unsigned i = 0; i < 3; ++i)
    f.bbs.push_back(new if_bb());

  f.bbs[0]->suc.push_back(f.bbs[1]);
  f.bbs[1]->suc.push_back(f.bbs[1]);
  f.bbs[1]->suc.push_back(f.bbs[2]);
  f.bbs[1]->pred.push_back(f.bbs[0]);
  f.bbs[1]->pred.push_back(f.bbs[1]);
  f.bbs[2]->pred.push_back(f.bbs[1]);

  f.bbs[0]->branch_pred = NULL;
  f.bbs[2]->branch_pred = NULL;

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[0]->t = u32();
  f.bbs[0]->vals[0]->op = VAL_CONST;
  to_vec_bool<32>(f.bbs[0]->vals[0]->const_val, 0);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[0]->t = u32();
  f.bbs[1]->vals[0]->op = VAL_PHI;

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[1]->t = u32();
  f.bbs[1]->vals[1]->op = VAL_PHI;

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[2]->t = u32();
  f.bbs[1]->vals[2]->op = VAL_CONST;
  to_vec_bool<32>(f.bbs[1]->vals[2]->const_val, 1103515245);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[3]->t = u32();
  f.bbs[1]->vals[3]->op = VAL_CONST;
  to_vec_bool<32>(f.bbs[1]->vals[3]->const_val, 12345);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[4]->t = u32();
  f.bbs[1]->vals[4]->op = VAL_MUL;
  f.bbs[1]->vals[4]->args.push_back(f.bbs[1]->vals[1]);
  f.bbs[1]->vals[4]->args.push_back(f.bbs[1]->vals[2]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[5]->t = u32();
  f.bbs[1]->vals[5]->op = VAL_ADD;
  f.bbs[1]->vals[5]->args.push_back(f.bbs[1]->vals[3]);
  f.bbs[1]->vals[5]->args.push_back(f.bbs[1]->vals[4]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[6]->t = u32();
  f.bbs[1]->vals[6]->op = VAL_CONST;
  to_vec_bool<32>(f.bbs[1]->vals[6]->const_val, 100);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[7]->t = u32();
  f.bbs[1]->vals[7]->op = VAL_XOR;
  f.bbs[1]->vals[7]->args.push_back(f.bbs[1]->vals[0]);
  f.bbs[1]->vals[7]->args.push_back(f.bbs[1]->vals[6]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[8]->t = bit_type();
  f.bbs[1]->vals[8]->op = VAL_OR_REDUCE;
  f.bbs[1]->vals[8]->args.push_back(f.bbs[1]->vals[7]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[9]->t = bit_type();
  f.bbs[1]->vals[9]->op = VAL_NOT;
  f.bbs[1]->vals[9]->args.push_back(f.bbs[1]->vals[8]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[10]->t = u32();
  f.bbs[1]->vals[10]->op = VAL_CONST;
  to_vec_bool<5>(f.bbs[1]->vals[10]->const_val, 1);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[11]->t = u32();
  f.bbs[1]->vals[11]->op = VAL_ADD;
  f.bbs[1]->vals[11]->args.push_back(f.bbs[1]->vals[0]);
  f.bbs[1]->vals[11]->args.push_back(f.bbs[1]->vals[10]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[12]->t = void_type();
  f.bbs[1]->vals[12]->op = VAL_SPAWN;
  f.bbs[1]->vals[12]->func_arg = "func";
  f.bbs[1]->vals[12]->args.push_back(f.bbs[1]->vals[5]);

  f.bbs[1]->branch_pred = f.bbs[1]->vals[9];

  f.bbs[2]->vals.push_back(new if_val());
  f.bbs[2]->vals[0]->t = void_type();
  f.bbs[2]->vals[0]->op = VAL_RET;
  f.bbs[2]->vals[0]->args.push_back(f.bbs[1]->vals[0]);

  f.bbs[1]->vals[0]->args.push_back(f.bbs[0]->vals[0]);
  f.bbs[1]->vals[0]->args.push_back(f.bbs[1]->vals[11]);

  f.bbs[1]->vals[1]->args.push_back(f.bbs[0]->vals[0]);
  f.bbs[1]->vals[1]->args.push_back(f.bbs[1]->vals[5]);
  
  f.bbs[0]->live_out.push_back(f.bbs[0]->vals[0]);
  f.bbs[1]->live_in.push_back(f.bbs[0]->vals[0]);
  f.bbs[1]->live_in.push_back(f.bbs[1]->vals[5]);
  f.bbs[1]->live_in.push_back(f.bbs[1]->vals[11]);
  f.bbs[1]->live_out.push_back(f.bbs[1]->vals[0]);
  f.bbs[1]->live_out.push_back(f.bbs[1]->vals[5]);
  f.bbs[1]->live_out.push_back(f.bbs[1]->vals[11]);
  f.bbs[2]->live_in.push_back(f.bbs[1]->vals[0]);

  int id = 0;
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    f.bbs[i]->id = i;
    for (unsigned j = 0; j < f.bbs[i]->vals.size(); ++j) {
      f.bbs[i]->vals[j]->bb = f.bbs[i];
      f.bbs[i]->vals[j]->id = id++;
    }
  }
}

void test_func(if_func &f) {
  // Test called function:
  // broadcast var: d1 (u3)
  // broadcast var: d2 (u3)
  // broadcast var: d3 (u3)
  // bb0:
  //  %0 = arg (u32)
  //  %1 = const #0 (u5)
  //  %2 = const #3 (u5)
  //  %3 = const #6 (u5)
  //  %4 = %0[%1, %2] (u3) -- dest
  //  %5 = %0[%2, %2] (u3) -- src0
  //  %6 = %0[%3, %2] (u3) -- src1
  //  %7 = ld_static @d1 (u3)
  //  %8 = ld_static @d2 (u3)
  //  %9 = ld_static @d3 (u3)
  //  %10 = xor %5, %7 (u3)
  //  %11 = xor %5, %8 (u3)
  //  %12 = xor %5, %9 (u3)
  //  %13 = xor %6, %7 (u3)
  //  %14 = xor %6, %8 (u3)
  //  %15 = xor %6, %9 (u3)
  //  %16 = or_reduce %10 (bit)
  //  %17 = or_reduce %11 (bit)
  //  %18 = or_reduce %12 (bit)
  //  %19 = or_reduce %13 (bit)
  //  %20 = or_reduce %14 (bit)
  //  %21 = or_reduce %15 (bit)
  //  %22 = cat %16, %17, %18, %19, %20, %21 (u6)
  //  %23 = ld_bcast_valid @d1
  //  %24 = ld_bcast_valid @d2
  //  %25 = ld_bcast_valid @d3
  //  %26 = cat %23, %24, %25, %23, %24, %25 (u6)
  //  %27 = not %22 (u6)
  //  %28 = and %26, %27 (u6)
  //  %29 = or_reduce %28 (bit)
  //  stall %29
  // bb1:
  //  st_static @d1, %4
  // bb2:
  //  st_static @d2, %4
  // bb3:
  //  st_static @d3, %4
  //  %34 = ret (void)
  f.static_vars["d1"] = u3bcastvar("d1");
  f.static_vars["d2"] = u3bcastvar("d2");
  f.static_vars["d3"] = u3bcastvar("d3");
  
  f.rtype = void_type();
  f.args.push_back(u32());

  for (unsigned i = 0; i < 4; ++i)
    f.bbs.push_back(new if_bb());

  f.bbs[0]->branch_pred = NULL;
  f.bbs[1]->branch_pred = NULL;
  f.bbs[2]->branch_pred = NULL;
  f.bbs[3]->branch_pred = NULL;

  f.bbs[0]->suc.push_back(f.bbs[1]);
  f.bbs[1]->suc.push_back(f.bbs[2]);
  f.bbs[2]->suc.push_back(f.bbs[3]);

  f.bbs[1]->pred.push_back(f.bbs[0]);
  f.bbs[2]->pred.push_back(f.bbs[1]);
  f.bbs[3]->pred.push_back(f.bbs[2]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[0]->t = u32();
  f.bbs[0]->vals[0]->op = VAL_ARG;

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[1]->t = uN(5);
  f.bbs[0]->vals[1]->op = VAL_CONST;
  to_vec_bool<32>(f.bbs[0]->vals[1]->const_val, 0);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[2]->t = uN(5);
  f.bbs[0]->vals[2]->op = VAL_CONST;
  to_vec_bool<32>(f.bbs[0]->vals[2]->const_val, 3);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[3]->t = uN(5);
  f.bbs[0]->vals[3]->op = VAL_CONST;
  to_vec_bool<32>(f.bbs[0]->vals[3]->const_val, 6);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[4]->t = uN(3);
  f.bbs[0]->vals[4]->op = VAL_LD_IDX;
  f.bbs[0]->vals[4]->args.push_back(f.bbs[0]->vals[0]);
  f.bbs[0]->vals[4]->args.push_back(f.bbs[0]->vals[1]);
  f.bbs[0]->vals[4]->args.push_back(f.bbs[0]->vals[2]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[5]->t = uN(3);
  f.bbs[0]->vals[5]->op = VAL_LD_IDX;
  f.bbs[0]->vals[5]->args.push_back(f.bbs[0]->vals[0]);
  f.bbs[0]->vals[5]->args.push_back(f.bbs[0]->vals[2]);
  f.bbs[0]->vals[5]->args.push_back(f.bbs[0]->vals[2]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[6]->t = uN(3);
  f.bbs[0]->vals[6]->op = VAL_LD_IDX;
  f.bbs[0]->vals[6]->args.push_back(f.bbs[0]->vals[0]);
  f.bbs[0]->vals[6]->args.push_back(f.bbs[0]->vals[3]);
  f.bbs[0]->vals[6]->args.push_back(f.bbs[0]->vals[2]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[7]->t = uN(3);
  f.bbs[0]->vals[7]->op = VAL_LD_STATIC;
  f.bbs[0]->vals[7]->static_arg = &f.static_vars["d1"];

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[8]->t = uN(3);
  f.bbs[0]->vals[8]->op = VAL_LD_STATIC;
  f.bbs[0]->vals[8]->static_arg = &f.static_vars["d2"];

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[9]->t = uN(3);
  f.bbs[0]->vals[9]->op = VAL_LD_STATIC;
  f.bbs[0]->vals[9]->static_arg = &f.static_vars["d3"];

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[10]->t = uN(3);
  f.bbs[0]->vals[10]->op = VAL_XOR;
  f.bbs[0]->vals[10]->args.push_back(f.bbs[0]->vals[5]);
  f.bbs[0]->vals[10]->args.push_back(f.bbs[0]->vals[7]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[11]->t = uN(3);
  f.bbs[0]->vals[11]->op = VAL_XOR;
  f.bbs[0]->vals[11]->args.push_back(f.bbs[0]->vals[5]);
  f.bbs[0]->vals[11]->args.push_back(f.bbs[0]->vals[8]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[12]->t = uN(3);
  f.bbs[0]->vals[12]->op = VAL_XOR;
  f.bbs[0]->vals[12]->args.push_back(f.bbs[0]->vals[5]);
  f.bbs[0]->vals[12]->args.push_back(f.bbs[0]->vals[9]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[13]->t = uN(3);
  f.bbs[0]->vals[13]->op = VAL_XOR;
  f.bbs[0]->vals[13]->args.push_back(f.bbs[0]->vals[6]);
  f.bbs[0]->vals[13]->args.push_back(f.bbs[0]->vals[7]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[14]->t = uN(3);
  f.bbs[0]->vals[14]->op = VAL_XOR;
  f.bbs[0]->vals[14]->args.push_back(f.bbs[0]->vals[6]);
  f.bbs[0]->vals[14]->args.push_back(f.bbs[0]->vals[8]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[15]->t = uN(3);
  f.bbs[0]->vals[15]->op = VAL_XOR;
  f.bbs[0]->vals[15]->args.push_back(f.bbs[0]->vals[6]);
  f.bbs[0]->vals[15]->args.push_back(f.bbs[0]->vals[9]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[16]->t = bit_type();
  f.bbs[0]->vals[16]->op = VAL_OR_REDUCE;
  f.bbs[0]->vals[16]->args.push_back(f.bbs[0]->vals[10]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[17]->t = bit_type();
  f.bbs[0]->vals[17]->op = VAL_OR_REDUCE;
  f.bbs[0]->vals[17]->args.push_back(f.bbs[0]->vals[11]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[18]->t = bit_type();
  f.bbs[0]->vals[18]->op = VAL_OR_REDUCE;
  f.bbs[0]->vals[18]->args.push_back(f.bbs[0]->vals[12]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[19]->t = bit_type();
  f.bbs[0]->vals[19]->op = VAL_OR_REDUCE;
  f.bbs[0]->vals[19]->args.push_back(f.bbs[0]->vals[13]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[20]->t = bit_type();
  f.bbs[0]->vals[20]->op = VAL_OR_REDUCE;
  f.bbs[0]->vals[20]->args.push_back(f.bbs[0]->vals[14]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[21]->t = bit_type();
  f.bbs[0]->vals[21]->op = VAL_OR_REDUCE;
  f.bbs[0]->vals[21]->args.push_back(f.bbs[0]->vals[15]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[22]->t = uN(6);
  f.bbs[0]->vals[22]->op = VAL_CONCATENATE;
  for (unsigned i = 0; i < 6; ++i)
    f.bbs[0]->vals[22]->args.push_back(f.bbs[0]->vals[16 + i]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[23]->t = bit_type();
  f.bbs[0]->vals[23]->op = VAL_LD_BCAST_VALID;
  f.bbs[0]->vals[23]->static_arg = &f.static_vars["d1"];

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[24]->t = bit_type();
  f.bbs[0]->vals[24]->op = VAL_LD_BCAST_VALID;
  f.bbs[0]->vals[24]->static_arg = &f.static_vars["d2"];

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[25]->t = bit_type();
  f.bbs[0]->vals[25]->op = VAL_LD_BCAST_VALID;
  f.bbs[0]->vals[25]->static_arg = &f.static_vars["d3"];

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[26]->t = uN(6);
  f.bbs[0]->vals[26]->op = VAL_CONCATENATE;
  for (unsigned j = 0; j < 2; ++j)
    for (unsigned i = 0; i < 3; ++i)
      f.bbs[0]->vals[26]->args.push_back(f.bbs[0]->vals[23 + i]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[27]->t = uN(6);
  f.bbs[0]->vals[27]->op = VAL_NOT;
  f.bbs[0]->vals[27]->args.push_back(f.bbs[0]->vals[22]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[28]->t = uN(6);
  f.bbs[0]->vals[28]->op = VAL_AND;
  f.bbs[0]->vals[28]->args.push_back(f.bbs[0]->vals[26]);
  f.bbs[0]->vals[28]->args.push_back(f.bbs[0]->vals[27]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[29]->t = bit_type();
  f.bbs[0]->vals[29]->op = VAL_OR_REDUCE;
  f.bbs[0]->vals[29]->args.push_back(f.bbs[0]->vals[28]);

  f.bbs[0]->stall = f.bbs[0]->vals[29];

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[0]->t = void_type();
  f.bbs[1]->vals[0]->op = VAL_ST_STATIC;
  f.bbs[1]->vals[0]->static_arg = &f.static_vars["d1"];
  f.bbs[1]->vals[0]->args.push_back(f.bbs[0]->vals[4]);

  f.bbs[2]->vals.push_back(new if_val());
  f.bbs[2]->vals[0]->t = void_type();
  f.bbs[2]->vals[0]->op = VAL_ST_STATIC;
  f.bbs[2]->vals[0]->static_arg = &f.static_vars["d2"];
  f.bbs[2]->vals[0]->args.push_back(f.bbs[0]->vals[4]);

  f.bbs[3]->vals.push_back(new if_val());
  f.bbs[3]->vals[0]->t = void_type();
  f.bbs[3]->vals[0]->op = VAL_ST_STATIC;
  f.bbs[3]->vals[0]->static_arg = &f.static_vars["d3"];
  f.bbs[3]->vals[0]->args.push_back(f.bbs[0]->vals[4]);

  f.bbs[3]->vals.push_back(new if_val());
  f.bbs[3]->vals[1]->t = void_type();
  f.bbs[3]->vals[1]->op = VAL_RET;

  f.bbs[0]->live_out.push_back(f.bbs[0]->vals[4]);
  f.bbs[1]->live_out.push_back(f.bbs[0]->vals[4]);
  f.bbs[2]->live_out.push_back(f.bbs[0]->vals[4]);
  f.bbs[1]->live_in.push_back(f.bbs[0]->vals[4]);
  f.bbs[2]->live_in.push_back(f.bbs[0]->vals[4]);
  f.bbs[3]->live_in.push_back(f.bbs[0]->vals[4]);
  
  unsigned id = 0;
  for (unsigned j = 0; j < f.bbs.size(); ++j) {
    f.bbs[j]->id = j;
    for (unsigned i = 0; i < f.bbs[j]->vals.size(); ++i, ++id) {
      f.bbs[j]->vals[i]->id = id;
      f.bbs[j]->vals[i]->bb = f.bbs[j];
    }
  }
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

  print(cout, p);

  ofstream chdl_out("cgen-9.chdl.cpp");
  gen_prog(chdl_out, p);

  ofstream cpp_out("cgen-9.sim.cpp");
  gen_prog_cpp(cpp_out, p);
  
  return 0;
}
