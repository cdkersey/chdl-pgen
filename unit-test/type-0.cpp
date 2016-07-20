#include <iostream>

#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"

using namespace bscotch;
using namespace std;

type u8() { return u(8); }
type u16() { return u(16); }
type u32() { return u(32); }

type s8() { return s(8); }
type s16() { return s(16); }
type s32() { return s(32); }

type fp_type(unsigned e, unsigned m) {
  type t = struct_type().
             add_field("s", bit()).
             add_field("e", u(e)).
             add_field("m", u(m));

  return t;
}

type cpx(unsigned e, unsigned m) {
  type t = struct_type().
             add_field("r", fp_type(e, m)).
             add_field("i", fp_type(e, m));

  return t;
}

type bsc_float() { return fp_type(8, 23); }
type cpx_float() { return cpx(8, 23); }
type cpx_double() { return cpx(11, 52); }

type cpx_double_1024() {
  type t = cpx_double();
  t.type_vec.push_back(TYPE_STATIC_ARRAY);
  t.type_vec.push_back(1024);
  return t;
}

void print_type(type (*f)()) {
  type t = f();
  cout << " === " << endl << "bpi:  ";
  print(cout, t);
  cout << endl << "chdl: " << type_chdl(t)
       << endl << "cpp:  " << type_cpp(t, "foo") << endl;
  cout << "size: " << t.size() << endl;
}

int main() {
  print_type(bit);
  print_type(u8);
  print_type(u16);
  print_type(u32);
  print_type(s8);
  print_type(s16);
  print_type(s32);
  print_type(bsc_float);
  print_type(cpx_float);
  print_type(cpx_double);
  print_type(cpx_double_1024);

  type t = cpx_float();
  cout << "Field 0 of cpx float: " << t.get_field_name(0) << endl;
  cout << "Field 1 of cpx float: " << t.get_field_name(1) << endl;
  cout << "Idx of r of cpx float: " << t.get_field_idx("r") << endl;
  cout << "Idx of i of cpx float: " << t.get_field_idx("i") << endl;
  cout << "Idx of s of cpx float: " << t.get_field_idx("s") << endl;
  type itype = t.get_field_type(t.get_field_idx("i")),
       rtype = t.get_field_type(t.get_field_idx("r"));
  cout << "Field i of cpx float type: " << endl;
  print(cout, itype);
  cout << endl;
  cout << "Field r of cpx float type: " << endl;
  print(cout, rtype);
  cout << endl;
  cout << "Field e of that: " << endl;
  type etype = rtype.get_field_type(rtype.get_field_idx("e"));
  print(cout, etype);
  cout << endl;
  
  return 0;
}
