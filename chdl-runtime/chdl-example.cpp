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

int main() {
  STATIC_VAR(main, x, ui<32>, 0x00000000);

  // Function main() interfaces
  typedef flit<chdl_void> main_call_t;
  typedef flit<chdl_void> main_ret_t;
  typedef flit<chdl_void> main_bb0_in_t;
  typedef flit<chdl_void> main_bb0_out_t;

  // main() function interfaces
  main_call_t main_call;
  main_ret_t main_ret;
  TAP(main_call);
  TAP(main_ret);

  // main() basic block 0 interfaces
  main_bb0_in_t main_bb0_in;
  vec<1, main_bb0_out_t> main_bb0_out_prebuf, main_bb0_out;
  TAP(main_bb0_in);
  TAP(main_bb0_out);

  // main() basic block 0
  vec<2, main_bb0_in_t> main_bb0_arb_in;
  _(main_bb0_arb_in[0], "valid") = _(main_call, "valid");
  _(main_call, "ready") = _(main_bb0_arb_in[0], "ready");

  _(main_bb0_arb_in[1], "valid") = _(main_bb0_out[0], "valid");
  _(main_call, "ready") = _(main_bb0_arb_in[1], "ready");

  Arbiter(main_bb0_in, ArbRR<2>, main_bb0_arb_in);

  node main_bb0_run(_(main_bb0_in, "valid") &&
    _(main_bb0_out_prebuf[0], "ready"));
  TAP(main_bb0_run);
  
  ui<32> main_0 = Lit<32>(0x00000001);
  ui<32> main_1 = LD_STATIC(main, x);
  ui<32> main_2 = main_1 + main_0;
  ST_STATIC(main, x, main_2, main_bb0_run);

  TAP(main_0);
  TAP(main_1);
  TAP(main_2);

  _(main_bb0_out_prebuf[0], "valid") = _(main_bb0_in, "valid");
  _(main_bb0_in, "ready") = _(main_bb0_out_prebuf[0], "ready");

  for (unsigned i = 0; i < 1; ++i)
    Buffer<1>(main_bb0_out[i], main_bb0_out_prebuf[i]);
  
  STATIC_VAR_GEN(main, x);

  // TODO: better starter
  _(main_call, "valid") = Wreg(Reg(_(main_call, "ready")), Lit(0), 1);
  
  optimize();
  
  ofstream vcd("chdl-example.vcd");
  run(vcd, 1000);
  
  return 0;
}
