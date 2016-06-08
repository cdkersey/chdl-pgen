#include <chdl/chdl.h>

#include <chdl/tristate.h>
#include <chdl/numeric.h>
#include <chdl/net.h>

using namespace std;
using namespace chdl;

template <typename T> struct staticvar {
  staticvar() { q = Wreg(wr, d); }
  staticvar(unsigned long v) { q = Wreg(wr, d, v); }
  
  T value() { return q; }

  void add_input(node wr, const T &x) {
    inputs.push_back(x);
    input_wr.push_back(wr);
  }

  void gen() {
    vector<node> cumulative_wr;
    cumulative_wr.push_back(Lit(0));
    for (unsigned i = 0; i <= input_wr.size(); ++i)
      cumulative_wr.push_back(input_wr[i] || cumulative_wr[i]);

    bus<sz<T>::value> b;
    for (unsigned i = 0; i < input_wr.size(); ++i) {
      b.connect(Flatten(inputs[i]), input_wr[i] && !cumulative_wr[i]);
      tap("input_wr", input_wr[i]);
      tap("cumulative_wr", cumulative_wr[i]);
    }
    Flatten(d) = b;
    TAP(b);
    wr = cumulative_wr[input_wr.size()];
  }
  
  node wr;
  T d, q;

  vector<node> input_wr;
  vector<T> inputs;
};

#define STATIC_VAR(func, var, type, initialval) \
  staticvar<type> func##_##var(initialval); \
  tap(#func "_" #var, func##_##var.value());

#define LD_STATIC(func, var) \
  (func##_##var.value())

#define ST_STATIC(func, var, val, wr)		\
  do { func##_##var.add_input(wr, val); } while (0)

#define STATIC_VAR_GEN(func, var) \
  do { func##_##var.gen(); } while (0)

typedef bvec<0> chdl_void;

// Output buffers
template <unsigned S, typename T>
  void BBOutputBuf(bvec<CLOG2(S)> sel, vec<S, flit<T> > &out, flit<T> &in)
{
  node full, fill, empty;
  
  bvec<S> r, v;
  for (unsigned i = 0; i < S; ++i)
    r[i] = _(out[i], "ready");

  node wr_buf = _(in, "ready") && _(in, "valid");

  bvec<CLOG2(S)> out_sel = Wreg(wr_buf, _(in, "sel"));
  T out_contents = Wreg(wr_buf, _(in, "contents"));

  Decoder(v, out_sel);
  
  for (unsigned i = 0; i < S; ++i) {
    _(out[i], "contents") = out_contents;
    _(out[i], "valid") = v[i] && full;
  }

  full = Reg((full && !empty) || fill);
  fill = wr_buf;
  empty = full && Mux(out_sel, r);
  _(in, "ready") = !full || empty;
}

template <typename T>
  void BBOutputBuf(vec<1, flit<T> > &out, flit<T> &in)
{
  Buffer<1>(out[0], in);
}

int main() {
  // // // // Begin generated code // // // //
  STATIC_VAR(main, x, ui<32>, 0x00000000);

  typedef flit<chdl_void > main_call_t;
  typedef flit<chdl_void > main_ret_t;
  main_call_t main_call;
  main_ret_t main_ret;

  // main BB 0 declarations
  typedef flit<chdl_void > main_bb0_in_t;
  typedef flit<chdl_void > main_bb0_out_t;
  main_bb0_in_t main_bb0_in;
  main_bb0_out_t main_bb0_out_prebuf;
  vec<1, main_bb0_out_t> main_bb0_out;
  vec<2, main_bb0_in_t> main_bb0_arb_in;

  // main BB 0 body
  _(main_bb0_arb_in[0], "valid") = _(main_bb0_out[0], "valid");
  _(main_bb0_arb_in[0], "contents") = _(main_bb0_out[0], "contents");
  _(main_bb0_out[0], "ready") = _(main_bb0_arb_in[0], "ready");
  _(main_bb0_arb_in[1], "valid") = _(main_call, "valid");
  _(main_bb0_arb_in[1], "contents") = _(main_call, "contents");
  _(main_call, "ready") = _(main_bb0_arb_in[1], "ready");
  Arbiter(main_bb0_in, ArbRR<2>, main_bb0_arb_in);
  node main_bb0_run(_(main_bb0_in, "valid") && _(main_bb0_out_prebuf, "ready"));
  ui<32> main_0 = Lit<32>(0x00000001);
  ui<32> main_1 = LD_STATIC(main, x);
  ui<32> main_2 = main_1 + main_0;
  ST_STATIC(main, x, main_2, main_bb0_run);
  _(main_bb0_out_prebuf, "valid") = _(main_bb0_in, "valid");
  _(main_bb0_in, "ready") = _(main_bb0_out_prebuf, "ready");
  BBOutputBuf(main_bb0_out, main_bb0_out_prebuf);

  STATIC_VAR_GEN(main, x);
  // // // // End generated code // // // //
  
  // TODO: better starter
  _(main_call, "valid") = Wreg(_(main_call, "ready"), Lit(0), 1);

  if (cycdet()) return 1;
  
  optimize();
  
  ofstream vcd("chdl-example.vcd");
  run(vcd, 1000);
  
  return 0;
}
