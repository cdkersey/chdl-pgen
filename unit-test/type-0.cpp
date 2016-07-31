#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <random>

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

type fp2_type(unsigned e, unsigned m) {
  type t = struct_type().
           add_field("sign", bit()).
           add_field("exponent", u(e)).
           add_field("mantissa", u(m));
  return t;
}

type cpx(unsigned e, unsigned m) {
  type t = struct_type().
             add_field("r", fp_type(e, m)).
             add_field("i", fp_type(e, m));

  return t;
}

type bsc_float() { return fp_type(8, 23); }
type bsc_float2() { return fp2_type(8, 23); }
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
       << endl << "cpp:  " << type_cpp(t) << endl;
  cout << "size: " << t.size() << endl;
}

int main() {
  vector<type> v;
  v.push_back(bit());
  v.push_back(bit());
  v.push_back(u(8));
  v.push_back(u(16));
  v.push_back(u(4));
  v.push_back(u(12));
  v.push_back(u(32));
  v.push_back(s(8));
  v.push_back(s(16));
  v.push_back(s(32));
  v.push_back(bsc_float());
  v.push_back(bsc_float2());
  v.push_back(bsc_float2());
  v.push_back(bsc_float());
  v.push_back(cpx_float());
  v.push_back(cpx_double());
  v.push_back(cpx_double_1024());

  shuffle(v.begin(), v.end(), default_random_engine(0));
  sort(v.begin(), v.end());

  cout << "Equivalence matrix: ";
  for (unsigned i = 0; i < v.size(); ++i) {
    for (unsigned j = 0; j < v.size(); ++j) {
      cout << ' ' << setw(3) << v[i].compare(v[j]);
    }
    cout << endl;
  }

  cout << endl << "Type strings:" << endl;
  for (auto &t : v) {
    cout << "chdl: " << type_chdl(t) << endl
         << "cpp: " << type_cpp(t) << endl;
  }

  return 0;
}
