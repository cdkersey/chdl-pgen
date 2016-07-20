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

type my_struct() {
  type t;

  t.type_vec.push_back(TYPE_STRUCT_BEGIN);
  t.type_vec.push_back(2);
  t.field_name[t.type_vec.size()] = "a";
  t.type_vec.push_back(TYPE_U);
  t.type_vec.push_back(32);
  t.type_vec.push_back(TYPE_FIELD_DELIM);
  t.field_name[t.type_vec.size()] = "b";
  t.type_vec.push_back(TYPE_U);
  t.type_vec.push_back(32);
  t.type_vec.push_back(TYPE_FIELD_DELIM);

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
  //  bb0:
  //    %0 = const #0 (u32 "a" u32 "b")
  //  bb1:
  //    %1 = phi %0 %10 (u32 "a" u32 "b")
  //    %2 = const #0 (u1)
  //    %3 = const #1 (u1)
  //    %4 = %1[%2] (u32)
  //    %5 = %1[%3] (u32)
  //    %6 = const #1 (u32)
  //    %7 = add %4, %6 (u32)
  //    %8 = sub %5, %6 (u32)
  //    %9 = st_idx %1, %2, %5 (u32 "a" u32 "b")
  //    %10 = st_idx %9, %3, %6 (u32 "a" u32 "b")
  //    br bb1
  
  f.rtype = void_type();

  for (unsigned i = 0; i < 2; ++i)
    f.bbs.push_back(new if_bb());

  f.bbs[0]->suc.push_back(f.bbs[1]);
  f.bbs[1]->suc.push_back(f.bbs[1]);
  f.bbs[1]->pred.push_back(f.bbs[0]);
  f.bbs[1]->pred.push_back(f.bbs[1]);

  f.bbs[0]->branch_pred = NULL;
  f.bbs[1]->branch_pred = NULL;

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[0]->t = my_struct();
  f.bbs[0]->vals[0]->op = VAL_CONST;
  to_vec_bool<32>(f.bbs[0]->vals[0]->const_val, 0);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[0]->t = my_struct();
  f.bbs[1]->vals[0]->op = VAL_PHI;

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[1]->t = uN(3);
  f.bbs[1]->vals[1]->op = VAL_CONST;
  to_vec_bool<5>(f.bbs[1]->vals[1]->const_val, 0);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[2]->t = uN(3);
  f.bbs[1]->vals[2]->op = VAL_CONST;
  to_vec_bool<5>(f.bbs[1]->vals[2]->const_val, 1);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[3]->t = u32();
  f.bbs[1]->vals[3]->op = VAL_LD_IDX;
  f.bbs[1]->vals[3]->args.push_back(f.bbs[1]->vals[0]);
  f.bbs[1]->vals[3]->args.push_back(f.bbs[1]->vals[1]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[4]->t = u32();
  f.bbs[1]->vals[4]->op = VAL_LD_IDX;
  f.bbs[1]->vals[4]->args.push_back(f.bbs[1]->vals[0]);
  f.bbs[1]->vals[4]->args.push_back(f.bbs[1]->vals[2]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[5]->t = u32();
  f.bbs[1]->vals[5]->op = VAL_CONST;
  to_vec_bool<32>(f.bbs[1]->vals[5]->const_val, 1);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[6]->t = u32();
  f.bbs[1]->vals[6]->op = VAL_ADD;
  f.bbs[1]->vals[6]->args.push_back(f.bbs[1]->vals[3]);
  f.bbs[1]->vals[6]->args.push_back(f.bbs[1]->vals[5]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[7]->t = u32();
  f.bbs[1]->vals[7]->op = VAL_SUB;
  f.bbs[1]->vals[7]->args.push_back(f.bbs[1]->vals[4]);
  f.bbs[1]->vals[7]->args.push_back(f.bbs[1]->vals[5]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[8]->t = my_struct();
  f.bbs[1]->vals[8]->op = VAL_ST_IDX;
  f.bbs[1]->vals[8]->args.push_back(f.bbs[1]->vals[0]);
  f.bbs[1]->vals[8]->args.push_back(f.bbs[1]->vals[1]);
  f.bbs[1]->vals[8]->args.push_back(f.bbs[1]->vals[6]);

  f.bbs[1]->vals.push_back(new if_val());
  f.bbs[1]->vals[9]->t = my_struct();
  f.bbs[1]->vals[9]->op = VAL_ST_IDX;
  f.bbs[1]->vals[9]->args.push_back(f.bbs[1]->vals[8]);
  f.bbs[1]->vals[9]->args.push_back(f.bbs[1]->vals[2]);
  f.bbs[1]->vals[9]->args.push_back(f.bbs[1]->vals[7]);

  f.bbs[1]->vals[0]->args.push_back(f.bbs[0]->vals[0]);
  f.bbs[1]->vals[0]->args.push_back(f.bbs[1]->vals[9]);
  
  f.bbs[0]->live_out.push_back(f.bbs[0]->vals[0]);
  f.bbs[1]->live_in.push_back(f.bbs[0]->vals[0]);
  f.bbs[1]->live_in.push_back(f.bbs[1]->vals[9]);
  f.bbs[1]->live_out.push_back(f.bbs[1]->vals[9]);

  int id = 0;
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    f.bbs[i]->id = i;
    for (unsigned j = 0; j < f.bbs[i]->vals.size(); ++j) {
      f.bbs[i]->vals[j]->bb = f.bbs[i];
      f.bbs[i]->vals[j]->id = id++;
    }
  }
}

void test_prog(if_prog &p) {
  // Test program:

  test_func(p.functions["bmain"]);
  break_cycles(p.functions["bmain"]);
}

int main() {
  if_prog p;

  test_prog(p);

  // print(cout, p);

  gen_prog(cout, p);
  
  return 0;
}
