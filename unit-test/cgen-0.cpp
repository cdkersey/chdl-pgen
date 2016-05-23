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

if_staticvar u32staticvar(unsigned initial = 0) {
  if_staticvar s;

  to_vec_bool<32>(s.initial_val, initial);

  s.t = u32();

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
  //    %30 = branch bb0

  f.static_vars["x"] = u32staticvar(0);

  f.bbs.push_back(if_bb());

  f.bbs[0].vals.push_back(if_val());
  f.bbs[0].vals[0].op = VAL_CONST;
  to_vec_bool<32>(f.bbs[0].vals[0].const_val, 1);

}

void test_prog(if_prog &p) {
  // Test program:

  test_func(p.functions["main"]);
}


int main() {
  if_prog p;

  test_prog(p);

  print(cout, p);

  return 0;
}
