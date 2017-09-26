#include <chdl/chdl.h>

#include <chdl/tristate.h>
#include <chdl/numeric.h>
#include <chdl/net.h>

#include "print_hex.h"
#include "late.h"
using namespace std;
using namespace chdl;

// Non-register global variable used for forwarding networks and similar.
template <typename T> struct bcastvar {
  void gen() {}

  node valid() { return v; }
  T value() { return val; }
  template <unsigned I>
    void add_input(node v_in, const T &x) { val = x; v = v_in; }
  
  T val;
  node v;
};

template <typename T, unsigned W> struct staticvar {
  staticvar() { q = Wreg(OrN(wr), Mux(Log2(wr), d)); } 
  staticvar(unsigned long v) { q = Wreg(OrN(wr), Mux(Log2(wr), d), v); }
  
  T value() { return q; }

  template <unsigned I> void add_input(node wr_in, const T &x) {
    wr[I] = wr_in;
    d[I] = x;
  }

  void gen() { }
  
  bvec<W> wr;
  vec<W, T> d;
  T q;
};

template <typename T, unsigned N, unsigned P> struct staticarray {
  staticarray() { }

  template <unsigned PORT>
    T ld(const bvec<N> &idx) {
      rd_a[PORT] = idx;
      return q[PORT];
    }

  void add_input(node wr_in, const bvec<N> &idx, const T &x) {
    wr_a = idx;
    d = x;
    wr = wr_in;
  }

  void gen() {
    vec<P, bvec<sz<T>::value> > q_raw = Syncmem(rd_a, Flatten(d), wr_a, wr);
    for (unsigned i = 0; i < P; ++i)
      Flatten(q[i]) = q_raw[i];
  }

  vec<P, bvec<N> > rd_a;
  bvec<N> wr_a;

  vec<P, T> q;
  
  node wr;
  T d;
};

#define BCAST_VAR(func, var, type) \
  bcastvar<type> func##_##var; \
  tap(#func "_" #var, func##_##var.value()); \
  tap(#func "_" #var "_valid", func##_##var.valid());

#define LD_STATIC(func, var) \
  (func##_##var.value())

#define LD_BCAST_VALID(func, var) \
  (func##_##var.valid())

#define ST_STATIC(func, var, val, wr, port) \
  do { func##_##var.add_input<(port)>(wr, val); } while (0)

#define LD_STATIC_ARRAY(func, var, idx, port) \
  (func##_##var.ld<(port)>(idx))

#define ST_STATIC_ARRAY(func, var, idx, val, wr) \
  do { func##_##var.add_input(wr, idx, val); } while (0)

#define STATIC_VAR_GEN(func, var) \
  do { func##_##var.gen(); } while (0)

typedef bvec<0> chdl_void;

template <unsigned M, unsigned N, typename T>
  vec<M, T> MultiMux(bvec<CLOG2(N)> sel, const vec<N, T> &in)
{
  vec<M, T> r;

  for (unsigned i = 0; i < M; ++i)
    r[i] = Mux(sel + Lit<CLOG2(N)>(i), in);
  
  return r;
}

template <unsigned N, typename T>
  vec<N, T> SingleRepl(bvec<CLOG2(N)> sel, const vec<N, T> &in, const T &rep)
{
  vec<N, T> r;

  for (unsigned i = 0; i < N; ++i)
    r[i] = Mux(sel == Lit<CLOG2(N)>(i), in[i], rep);
  
  return r;
}

// Output buffers offer no storage/delay.
template <typename T> void BBOutputBuf(flit<T> &out, flit<T> &in) {
  _(in, "ready") = _(out, "ready");
  _(out, "valid") = _(in, "valid");
  _(out, "contents") = _(in, "contents");
}

template <unsigned S, typename T>
  void BBOutputSpawnBuf(vec<S, flit<T> > &out, flit<T> &in)
{
  bvec<S> ready_signals;
  for (unsigned i = 0; i < S; ++i)
    ready_signals[i] = _(out[i], "ready");

  _(in, "ready") = AndN(ready_signals);
  for (unsigned i = 0; i < S; ++i) {
    bvec<S - 1> other_readys;
    for (unsigned j = 0; j < S; ++j) {
      unsigned idx = (j >= i) ? j - 1 : j;
      if (j != i) other_readys[idx] = ready_signals[j];
    }
    _(out[i], "valid") = _(in, "valid") && AndN(other_readys);;
    _(out[i], "contents") = _(in, "contents");
  }

}

template <unsigned S, typename T>
  void BBOutputBuf(bvec<CLOG2(S)> sel, vec<S, flit<T> > &out, flit<T> &in)
{
  HIERARCHY_ENTER();

  typedef ag<STP("sel"), bvec<CLOG2(S)>,
          ag<STP("x"), T> > wrapped_in_t;

  flit<wrapped_in_t> buf_in, buf_out;

  _(in, "ready") = _(buf_in, "ready");
  _(buf_in, "valid") = _(in, "valid");
  _(_(buf_in, "contents"), "sel") = sel;
  _(_(buf_in, "contents"), "x") = _(in, "contents");
  
  BBOutputBuf(buf_out, buf_in);

  bvec<S> r, v;
  for (unsigned i = 0; i < S; ++i)
    r[i] = _(out[i], "ready");
  _(buf_out, "ready") = Mux(_(_(buf_out, "contents"), "sel"), r);
  v = Decoder(_(_(buf_out, "contents"), "sel"));
  for (unsigned i = 0; i < S; ++i)
    _(out[i], "valid") = v[i] && _(buf_out, "valid");

  for (unsigned i = 0; i < S; ++i)
    _(out[i], "contents") = _(_(buf_out, "contents"), "x");
  
  HIERARCHY_EXIT();
}

template <typename T>
  void BBOutputBuf(vec<1, flit<T> > &out, flit<T> &in)
{
  BBOutputBuf(bvec<0>(), out, in);
}

template <typename T>
  void BBInputBuf(flit<T> &out, flit<T> &in)
{
  HIERARCHY_ENTER();

  node full, fill, empty;
  node wr_buf = _(in, "ready") && _(in, "valid");

  _(out, "contents") = Wreg(wr_buf, _(in, "contents"));
  _(out, "valid") = full;

  full = Reg((full && !empty) || fill);
  fill = wr_buf;
  empty = full && _(out, "ready");
  _(in, "ready") = !full || empty;

  HIERARCHY_EXIT();
}

template <typename T, unsigned N>
  void BBArbiter(flit<T> &out, vec<N, flit<T> > &in)
{
  bvec<N> v, cv, uv(v & ~cv);
  for (unsigned i = 0; i < N; ++i) v[i] = _(in[i], "valid");
  
  cv[0] = Lit(0);
  for (unsigned i = 1; i < N; ++i)
    cv[i] = v[i - 1] || cv[i - 1];

  for (unsigned i = 0; i < N; ++i)
    _(in[i], "ready") = !cv[i] && _(out, "ready");
  bvec<CLOG2(N)> sel(Log2(uv));

  vec<N, T> contents_in;
  for (unsigned i = 0; i < N; ++i) contents_in[i] = _(in[i], "contents");
  
  _(out, "valid") = OrN(v);
  _(out, "contents") = Mux(sel, contents_in);
}

template <typename T>
  void BBInputBuf2(flit<T> &out, flit<T> &in)
{
  HIERARCHY_ENTER();

  node fill_a, fill_b, empty_a, empty_b, a_full, b_full, b_sel;

  T a = Wreg(fill_a, _(in, "contents")), b = Wreg(fill_b, _(in, "contents"));

  empty_a = !b_sel && _(out, "ready") && a_full;
  empty_b = b_sel && _(out, "ready") && b_full;

  fill_a = !a_full && _(in, "valid") && _(in, "ready");
  fill_b = a_full && _(in, "valid") && _(in, "ready");

  a_full = Reg(fill_a || (a_full && !empty_a));
  b_full = Reg(fill_b || (b_full && !empty_b));

  b_sel = b_full;

  _(in, "ready") = !a_full || !b_full;

  _(out, "contents") = Mux(b_sel, a, b);
  _(out, "valid") = a_full || b_full;  

  HIERARCHY_EXIT();
}

struct generic_global {};
template <typename T> struct specific_global : public generic_global {
  specific_global(): contents() {}
  template <typename U> specific_global(const U &x): contents(x) {}
  T &get() { return contents; }

  T contents;
};

#include "cgen-out.incl"

int main() {
  // Use a simple payload to verify that live values work.
  bmain_call_t<bvec<32> > bmain_call;
  bmain_ret_t<bvec<32> > bmain_ret;

  _(_(bmain_call, "contents"), "live") = Lit<32>(0x5701ca55);
  
  // Start by calling bmain.
  _(bmain_call, "valid") = Wreg(_(bmain_call, "ready"), Lit(0), 1);
 
  // Always ready for return.
  _(bmain_ret, "ready") = Lit(1);
  
  TAP(bmain_call);
  TAP(bmain_ret);

  bmain(bmain_ret, bmain_call);
  
  if (cycdet()) return 1;

  optimize();

  std::cout << "Critical path: " << critpath() << " gates" << endl;
  
  ofstream vcd("chdl-example.vcd");
  run(vcd, 10000);

  ofstream nand("chdl-example.nand");
  print_netlist(nand);

  ofstream vl("chdl-example.v");
  print_verilog("pgenmod", vl, true);

  return 0;
}
