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

type uN(int n) { return u(n); }
type u32() { return u(32); }

type bit_type() {
  type t;
  t.type_vec.push_back(TYPE_BIT);

  return t;
}

void test_bmain(if_func &f) {
  // Test function:
  //  bb0:
  //    %0 = const #0 (u32)
  //  bb1:
  //    %1 = phi %0, %7 (u32)
  //    %2 = const #10 (u32)
  //    %3 = xor %1, %2 (u32)
  //    %4 = or_reduce %3 (bit)
  //    %5 = not %4 (bit)
  //    %6 = const #1 (u32)
  //    %7 = add %6, %1
  //    %8 = spawn "func", %1
  //    br %5, bb1, bb2
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

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[0]->t = u32();
  f.bbs[0]->vals[0]->op = VAL_CONST;
  to_vec_bool<32>(f.bbs[0]->vals[0]->const_val, 0);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[0]->t = u32();
  f.bbs[1]->vals[0]->op = VAL_PHI;

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[1]->t = u32();
  f.bbs[1]->vals[1]->op = VAL_CONST;
  to_vec_bool<5>(f.bbs[1]->vals[1]->const_val, 10);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[2]->t = u32();
  f.bbs[1]->vals[2]->op = VAL_XOR;
  f.bbs[1]->vals[2]->args.push_back(f.bbs[1]->vals[0]);
  f.bbs[1]->vals[2]->args.push_back(f.bbs[1]->vals[1]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[3]->t = bit_type();
  f.bbs[1]->vals[3]->op = VAL_OR_REDUCE;
  f.bbs[1]->vals[3]->args.push_back(f.bbs[1]->vals[2]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[4]->t = bit_type();
  f.bbs[1]->vals[4]->op = VAL_NOT;
  f.bbs[1]->vals[4]->args.push_back(f.bbs[1]->vals[3]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[5]->t = u32();
  f.bbs[1]->vals[5]->op = VAL_CONST;
  to_vec_bool<5>(f.bbs[1]->vals[5]->const_val, 1);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[6]->t = u32();
  f.bbs[1]->vals[6]->op = VAL_ADD;
  f.bbs[1]->vals[6]->args.push_back(f.bbs[1]->vals[0]);
  f.bbs[1]->vals[6]->args.push_back(f.bbs[1]->vals[5]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[7]->t = void_type();
  f.bbs[1]->vals[7]->op = VAL_SPAWN;
  f.bbs[1]->vals[7]->func_arg = "func";
  f.bbs[1]->vals[7]->args.push_back(f.bbs[1]->vals[0]);

  f.bbs[1]->branch_pred = f.bbs[1]->vals[4];
  
  f.bbs[2]->vals.push_back(new if_val());
  f.bbs[2]->vals[0]->t = void_type();
  f.bbs[2]->vals[0]->op = VAL_RET;
  f.bbs[2]->vals[0]->args.push_back(f.bbs[1]->vals[0]);

  f.bbs[1]->vals[0]->args.push_back(f.bbs[0]->vals[0]);
  f.bbs[1]->vals[0]->args.push_back(f.bbs[1]->vals[6]);
  
  f.bbs[0]->live_out.push_back(f.bbs[0]->vals[0]);
  f.bbs[1]->live_in.push_back(f.bbs[0]->vals[0]);
  f.bbs[1]->live_in.push_back(f.bbs[1]->vals[6]);
  f.bbs[1]->live_out.push_back(f.bbs[1]->vals[0]);
  f.bbs[1]->live_out.push_back(f.bbs[1]->vals[6]);
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
  // bb0:
  //  %0 = arg (u32)
  //  %1 = const #5 (u32)
  //  %2 = xor %0, %1 (u32)
  //  %3 = or_reduce %2 (bit)
  //  %4 = not %3 (bit)
  //  %5 = ret %4 (void)
  f.rtype = bit_type();
  f.args.push_back(u32());
  
  f.bbs.push_back(new if_bb());
  f.bbs[0]->id = 0;
  f.bbs[0]->branch_pred = NULL;

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[0]->t = u32();
  f.bbs[0]->vals[0]->op = VAL_ARG;

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[1]->t = u32();
  f.bbs[0]->vals[1]->op = VAL_CONST;
  to_vec_bool<32>(f.bbs[0]->vals[1]->const_val, 5);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[2]->t = u32();
  f.bbs[0]->vals[2]->op = VAL_XOR;
  f.bbs[0]->vals[2]->args.push_back(f.bbs[0]->vals[0]);
  f.bbs[0]->vals[2]->args.push_back(f.bbs[0]->vals[1]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[3]->t = bit_type();
  f.bbs[0]->vals[3]->op = VAL_OR_REDUCE;
  f.bbs[0]->vals[3]->args.push_back(f.bbs[0]->vals[2]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[4]->t = bit_type();
  f.bbs[0]->vals[4]->op = VAL_NOT;
  f.bbs[0]->vals[4]->args.push_back(f.bbs[0]->vals[3]);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[5]->t = void_type();
  f.bbs[0]->vals[5]->op = VAL_RET;
  f.bbs[0]->vals[5]->args.push_back(f.bbs[0]->vals[4]);

  for (unsigned i = 0; i < f.bbs[0]->vals.size(); ++i) {
    f.bbs[0]->vals[i]->id = i;
    f.bbs[0]->vals[i]->bb = f.bbs[0];
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

  // print(cout, p);

  gen_prog(cout, p);
  
  return 0;
}
