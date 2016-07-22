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

const unsigned B(8), N(4), A(30), I(27);

void bmain() {
  type req_t = mem_req(B, N, A, I), resp_t = mem_resp(B, N, I);

  var i(u(32)), seed(u(32)), r(u(32)), req(req_t), resp(resp_t);

  function("bmain");
  label("entry");
  i = 0_c;
  seed = 1_c;
  cat(req)(lit(u(req_t.size()), 0));
  cat(resp)(lit(u(resp_t.size()), 0));

  label("loop");
  i = i + 1_c;
  seed = seed * 1103515245_c + 12345_c;
  req = repl(req, "wr", load(seed, 30_c) & load(seed, 31_c));
  req = repl(req, "llsc", 0_c);
  cat(r)(lit(u(17), 0))(load(seed, 16_c, 15));

  br(i < 100_c)("end")("loop");

  label("end");
  ret();
}

void req_thread() {
  function("req_thread");
}

void mem_unit() {
  function("mem_unit");
}

using namespace std;

int main(int argc, char **argv) {
  if_prog p;
  asm_prog a(p);
  init_macro_env(a);

  bmain();
  //req_thread();
  //mem_unit();
  
  finish_macro_env();

  print(cout, p); 
  gen_prog(cout, p);

  return 0;
}
