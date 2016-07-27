#include <iostream>
#include <fstream>

#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"
#include "../cgen.h"
#include "../cgen-cpp.h"
#include "../break_cycles.h"
#include "../prevent_deadlock.h"

using namespace bscotch;
using namespace std;

template <unsigned N> void to_vec_bool(vector<bool> &v, unsigned long x) {
  v.clear();
  for (unsigned i = 0; i < N; ++i)
    v.push_back(x & (1ul<<i));
}

type u32() { return u(32); }

if_staticvar u32staticvar(string name, unsigned initial = 0) {
  if_staticvar s;

  s.name = name;
  to_vec_bool<32>(s.initial_val, initial);

  s.t = u32();

  s.broadcast = false;

  return s;
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
  
  f.rtype = void_type();

  f.bbs.push_back(new if_bb());
  f.bbs[0]->id = 0;

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[0]->t = u32();
  f.bbs[0]->vals[0]->op = VAL_CONST;
  f.bbs[0]->vals[0]->static_arg = NULL;
  f.bbs[0]->vals[0]->id = 0;
  f.bbs[0]->vals[0]->bb = f.bbs[0];
  to_vec_bool<32>(f.bbs[0]->vals[0]->const_val, 1);

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[1]->t = u32();
  f.bbs[0]->vals[1]->op = VAL_LD_STATIC;
  f.bbs[0]->vals[1]->id = 1;
  f.bbs[0]->vals[1]->bb = f.bbs[0];
  f.bbs[0]->vals[1]->static_arg = &f.static_vars["x"];

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[2]->t = u32();
  f.bbs[0]->vals[2]->op = VAL_ADD;
  f.bbs[0]->vals[2]->args.push_back(f.bbs[0]->vals[1]);
  f.bbs[0]->vals[2]->args.push_back(f.bbs[0]->vals[0]);
  f.bbs[0]->vals[2]->static_arg = NULL;
  f.bbs[0]->vals[2]->bb = f.bbs[0];
  f.bbs[0]->vals[2]->id = 2;

  f.bbs[0]->vals.push_back(new if_val());
  f.bbs[0]->vals[3]->op = VAL_ST_STATIC;
  f.bbs[0]->vals[3]->static_arg = &f.static_vars["x"];
  f.bbs[0]->vals[3]->args.push_back(f.bbs[0]->vals[2]);
  f.bbs[0]->vals[3]->id = 3;
  f.bbs[0]->vals[3]->bb = f.bbs[0];

  f.bbs[0]->suc.push_back(f.bbs[0]);
  f.bbs[0]->pred.push_back(f.bbs[0]);
  f.bbs[0]->branch_pred = NULL;
}

void test_prog(if_prog &p) {
  // Test program:

  test_func(p.functions["bmain"]);
  break_cycles(p.functions["bmain"]);
  prevent_deadlock(p.functions["bmain"]);
}

int main() {
  if_prog p;

  test_prog(p);

  print(cout, p);

  gen_prog(cout, p);

  ofstream out("cgen-0.sim.cpp");
  gen_prog_cpp(out, p);
  
  return 0;
}
