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

type cpx(unsigned e, unsigned m) {
  type t;

  t.type_vec.push_back(TYPE_STRUCT_BEGIN);
  t.type_vec.push_back(2);
  t.field_name[t.type_vec.size()] = "r";
  t.type_vec.push_back(TYPE_STRUCT_BEGIN);
  t.type_vec.push_back(3);
  t.field_name[t.type_vec.size()] = "s";
  t.type_vec.push_back(TYPE_BIT);
  t.type_vec.push_back(TYPE_FIELD_DELIM);
  t.field_name[t.type_vec.size()] = "e";
  t.type_vec.push_back(TYPE_U);
  t.type_vec.push_back(e);
  t.type_vec.push_back(TYPE_FIELD_DELIM);
  t.field_name[t.type_vec.size()] = "m";
  t.type_vec.push_back(TYPE_U);
  t.type_vec.push_back(m);
  t.type_vec.push_back(TYPE_FIELD_DELIM);
  t.type_vec.push_back(TYPE_FIELD_DELIM);
  t.field_name[t.type_vec.size()] = "i";
  t.type_vec.push_back(TYPE_STRUCT_BEGIN);
  t.type_vec.push_back(3);
  t.field_name[t.type_vec.size()] = "s";
  t.type_vec.push_back(TYPE_BIT);
  t.type_vec.push_back(TYPE_FIELD_DELIM);
  t.field_name[t.type_vec.size()] = "e";
  t.type_vec.push_back(TYPE_U);
  t.type_vec.push_back(e);
  t.type_vec.push_back(TYPE_FIELD_DELIM);
  t.field_name[t.type_vec.size()] = "m";
  t.type_vec.push_back(TYPE_U);
  t.type_vec.push_back(m);
  t.type_vec.push_back(TYPE_FIELD_DELIM);
  t.type_vec.push_back(TYPE_FIELD_DELIM);
  
  return t;
}

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
  print_type(cpx_float);
  print_type(cpx_double);
  print_type(cpx_double_1024);

  return 0;
}
