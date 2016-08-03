#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"
#include "../cgen.h"
#include "../cgen-cpp.h"
#include "../break_cycles.h"
#include "../asm-macro.h"

using namespace bscotch;

type mem_req(int b, int n, int a, int i) {
  type t = struct_type().
    add_field("d", sa(u(b), n)).
    add_field("a", u(a)).
    add_field("mask", u(n)).
    add_field("wr", bit()).
    add_field("llsc", bit()).
    add_field("id", u(i));

  return t;
}

type mem_resp(int b, int n, int i) {
  type t = struct_type().
    add_field("q", sa(u(b), n)).
    add_field("llsc_suc", bit()).
    add_field("wr", bit()).
    add_field("id", u(i));
  return t;
}

int clog2(unsigned long x) {
  int add = (x & (x - 1)) ? 1 : 0;
  
  for (int i = 63; i != -1; --i)
    if (x & (1ull << i)) return i + add;

  return 0;
}

void scratchpad(int B, int N, int A, int I, int SZ) {
  std::vector<std::string> sram_names;
  for (unsigned i = 0; i < N; ++i) {
    using namespace std;
    ostringstream oss;
    oss << "sram" << setw(2) << setfill('0') << i;
    sram_names.push_back(oss.str());
  }

  function("scratchpad");
  
  for (unsigned i = 0; i < N; ++i)
    static_var(sram_names[i].c_str(), a(u(B), (1<<SZ)));
  
  var req(mem_req(B, N, A, I)), resp(mem_resp(B, N, I)),
      addr(u(SZ)), d(sa(u(B), N)), q(sa(u(B), N)),
      wr(bit()), mask(u(N)), id(u(I));
  
  label("entry");
  resp = lit(u(mem_resp(B, N, I).size()), 0);
  req = arg(mem_req(B, N, A, I));

  addr = load(load(req, "a"), lit(u(32), 0), lit(u(32), SZ));
  d = load(req, "d");
  wr = load(req, "wr");
  mask = load(req, "mask");
  id = load(req, "id");

  q = lit(u(N*B), 0);

  for (unsigned i = 0; i < N; ++i) {
    var p(bit());
    p = load(mask, lit(u(32), i)) & wr;

    // Perform store for this byte.
    store(sram_names[i].c_str(), addr, load(d, lit(u(32), i))); pred(p);

    // Perform load for this byte.
    q = repl(q, lit(u(clog2(N)), i), load(sram_names[i].c_str(), addr));
  }

  label("exit_scratchpad");
  resp = repl(resp, "q", q);
  resp = repl(resp, "id", id);
  resp = repl(resp, "wr", wr);
  
  ret(resp);
}

const unsigned B(8), N(4), A(30), I(16), SZ(5);

void bmain() {
  type req(mem_req(B, N, A, I)), resp(mem_resp(B, N, I));
  
  
  // Function bmain() : spawn 10 threads in tmain instance.
  function("bmain");
  var i(u(32));

  label("entry");

  i = lit(u(32), 0);
  
  label("loop");
  spawn("tmain")(i); // Must be last in basic block.

  label("loop2");
  i = i + lit(u(32), 1);
  br(i == lit(u(32), 100))("loop")("exit");
  
  label("exit");
  ret();
}

void tmain() {
  function("tmain");

  var tid(u(32));
  var req(mem_req(B, N, A, I)), resp(mem_resp(B, N, I)), d(sa(u(8), 4));

  label("entry");  
  tid = arg(u(32));
  spawn("print_hex")(tid);
  br(tid < lit(u(32), 50))("do_read")("do_write");

  label("do_read");
  
  req = lit(u(mem_req(B, N, A, I).size()), 0);
  req = repl(req, "wr", lit(bit(), 0));
  req = repl(req, "id", load(tid, lit(u(5), 0), lit(u(6), I)));
  req = repl(req, "a", load(tid, lit(u(5), 0), lit(u(6), A)));

  br("do_call");

  label("do_write");

  d = lit(u(32), 0);
  d = repl(d, lit(u(2), 0), load(tid, lit(u(5), 0), lit(u(5), 8)));
  
  req = lit(u(mem_req(B, N, A, I).size()), 0);
  req = repl(req, "wr", lit(bit(), 1));
  req = repl(req, "mask", lit(u(4), 0xf));
  req = repl(req, "d", d);
  req = repl(req, "id", load(tid, lit(u(5), 0), lit(u(6), I)));
  req = repl(req, "a", load(tid, lit(u(5), 0), lit(u(6), A)));
  
  label("do_call");
  call("scratchpad", resp)(req);

  label("after_call");
  var i(u(8));
  i = load(load(resp, "q"), lit(u(2), 0));
  var printval_1(u(32)), printval_2(u(32));
  cat(printval_1)(lit(u(32 - I), 0))(load(resp, "id"));
  cat(printval_2)(lit(u(24), 0))(i);
  br(tid < lit(u(32), 50))("first_print")("exit");
  
  label("first_print");
  spawn("print_hex2")(printval_1)(printval_2);

  #if 1
  label("loop");
  var j(u(8));
  j = i;
  i = i - lit(u(8), 1);
  br(j == lit(u(8), 0))("loop")("exit");
  #endif
  
  label("exit");
  
  ret();
}

int main(int argc, char **argv) {
  using namespace std;

  if_prog p;
  asm_prog a(p);
  init_macro_env(a);

  bmain();
  tmain();
  scratchpad(B, N, A, I, SZ);

  finish_macro_env();

  print(std::cout, p);

  ofstream chdl_out("macro-4.chdl.cpp");
  gen_prog(chdl_out, p);

  ofstream cpp_out("macro-4.sim.cpp");
  gen_prog_cpp(cpp_out, p);

  return 0;
}
