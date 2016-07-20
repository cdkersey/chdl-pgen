#include <iostream>

#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"
#include "../cgen.h"
#include "../break_cycles.h"
#include "../asm-macro.h"

using namespace bscotch;
using namespace std;

type mem_req(unsigned B, unsigned N, unsigned A, unsigned I) {
  return struct_type().
    add_field("wr", bit()).
    add_field("llsc", bit()).
    add_field("mask", u(N)).
    add_field("addr", u(A)).
    add_field("d", a(u(B), N)).
    add_field("id", u(I));
}

type mem_resp(unsigned B, unsigned N, unsigned I) {
  return struct_type().
    add_field("wr", bit()).
    add_field("llsc", bit()).
    add_field("q", a(u(B), N)).
    add_field("id", u(I));
}

const usnigned B(8), N(4), A(30), I(27);

void bmain() {
  type req_t = mem_req(B, N, A, I), resp_t = mem_req(B, N, I);

  function("bmain");

  label("entry");

  label("start");

  br("start");
}

void req_thread() {
  function("req_thread");
}

void mem_unit() {
  function("mem_unit");
}

int main(int argc, char **argv) {
  if_prog p;
  asm_prog a(p);
  init_macro_env(a);

  bmain();
  req_thread();
  mem_unit();
  
  finish_macro_env();

  print(cout, p); 
  gen_prog(cout, p);

  return 0;
}
