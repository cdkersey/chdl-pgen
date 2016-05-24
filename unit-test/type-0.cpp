#include <iostream>

#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"

using namespace bscotch;
using namespace std;

type u_n(unsigned n) {
  type t;
  t.type_vec.push_back(TYPE_U);
  t.type_vec.push_back(n);

  return t;  
}

type s_n(unsigned n) {
  type t;
  t.type_vec.push_back(TYPE_S);
  t.type_vec.push_back(n);

  return t;  
}

type bit() {
  type t;
  t.type_vec.push_back(TYPE_BIT);
  return t;
}

type u8() { return u_n(8); }
type u16() { return u_n(16); }
type u32() { return u_n(32); }

type s8() { return s_n(8); }
type s16() { return s_n(16); }
type s32() { return s_n(32); }

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
type cpx_double() { return cpx(11, 53); }

void print_type(type (*f)()) {
  type t = f();
  print(cout, t);
  cout << endl;
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

  return 0;
}
