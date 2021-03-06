#ifndef CHDL_PGEN_RUNTIME_CPP_H
#define CHDL_PGEN_RUNTIME_CPP_H

#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <map>

namespace pgen {
  namespace runtime {
    template <unsigned N> struct ui;
    template <unsigned N> struct si;

template <unsigned N, typename T> struct array {
  array() { for (unsigned i = 0; i < N; ++i) contents[i] = 0; }

  array &operator=(const array &r) {
    for (unsigned i = 0; i < N; ++i)
      contents[i] = r.contents[i];
    return *this;
  }

  array &operator=(const si<N> &r);
  array &operator=(const ui<N> &r);

  array &operator=(unsigned long r);

  operator T*() { return contents; }
  operator T const *() const { return contents; }
  
  T contents[N];
};

template <unsigned N> struct ui {
  ui(): val(0) {}
  ui(unsigned long val): val(val) {}
  operator unsigned long() const { return val; }
  ui &operator=(unsigned long x) { val = x & ((1ull<<N)-1); return *this; }
  ui &operator=(array<N, bool> &v) {
    val = 0;
    for (unsigned i = 0; i < N; ++i)
      if (v[i]) val |= (1ull<<i);
    return *this;
  }
  unsigned long val;
};

template <unsigned N> struct si {
  si(): val(0) {}
  si(long val): val(val) {}
  operator long() const {
    if ((val>>(N-1))&1) return val | ~((1ull<<N)-1);
    else return val;
  }
  si &operator=(long x) { val = (x & ((1ull<<N) - 1)); return *this; }
  si &operator=(array<N, bool> &v) {
    val = 0;
    for (unsigned i = 0; i < N; ++i)
      if (v[i]) val |= (1ull<<i);
    return *this;
  }
  long val;
};

template <unsigned N, typename T>
  array<N, T> &array<N, T>::operator=(const si<N> &r)
{
  for (unsigned i = 0; i < N; ++i)
    contents[i] = (r & (1ull<<i)) ? 1 : 0;
  
  return *this;
}

template <unsigned N, typename T>
  array<N, T> &array<N, T>::operator=(const ui<N> &r)
{
  for (unsigned i = 0; i < N; ++i)
    contents[i] = (r & (1ull<<i)) ? 1 : 0;

  return *this;
}

template <unsigned N, typename T>
  array<N, T> &array<N, T>::operator=(unsigned long r)
{
  for (unsigned i = 0; i < N; ++i)
    contents[i] = (r & (1ull<<i)) ? 1 : 0;

  return *this;
}

template <unsigned N, typename T>
  std::ostream &operator<<(std::ostream &out, const array<N, T> &a)
{
  out << '{';
  for (unsigned i = 0; i < N; ++i) {
    if (i != 0) out << ", ";
    out << a[i];
  }
  out << '}';

  return out;
}

static bool or_reduce(const ui<0> &x) { return false; }
static bool and_reduce(const ui<0> &x) { return true; }

static bool or_reduce(const ui<1> &x) { return x&1; }
static bool and_reduce(const ui<1> &x) { return x&1; }

template <unsigned N> bool or_reduce(const ui<N> &x) {
  ui<N/2> a = x & ((1<<N/2)-1);
  ui<N - N/2> b = (x >> N/2) & ((1<<(N-N/2))-1);
  return or_reduce(a) || or_reduce(b);
}

template <unsigned N> bool and_reduce(const ui<N> &x) {
  ui<N/2> a = x & ((1<<N/2)-1);
  ui<N - N/2> b = (x >> N/2) & ((1<<(N-N/2))-1);
  return and_reduce(a) && and_reduce(b);
}

template <typename T> struct concatenator {
  concatenator(T &out): out(out), bit_pos(0) { out = 0; }
  
  concatenator operator()(const T &x) {
    out = x;
    return *this;
  }

  template <unsigned N> concatenator operator()(const ui<N> &x) {
    out = out << N;
    out = out | (x & ((1<<N)-1));
    bit_pos += N;

    return *this;
  }

  concatenator operator()(const bool &x) {
    out = out << 1;
    out = out | (x ? 1 : 0);
    bit_pos++;

    return *this;
  }
  
  T &out;
  unsigned bit_pos;
};

template <> struct concatenator<bool> {
  concatenator(bool &out): out(out) {}
  
  concatenator operator()(bool &x) {
    out = x;
    return *this;
  }

  bool &out;
};

template <typename T> concatenator<T> cat(T &out) {
  return concatenator<T>(out);
}

std::map<unsigned, array<16, ui<32> > > mem_state;

// mem_write declarations.
struct mem_write_call_t {
  bool valid;
  void *live;
  ui<26> arg0;
  array<16, ui<32> > arg1;
};

struct mem_write_ret_t {
  bool valid;
  void *live;
};

struct mem_write_state_t {
  mem_write_call_t call;
  mem_write_ret_t ret;
};

static void init_mem_write(mem_write_state_t &s) {
  s.call.valid = false;
  s.ret.valid = false;
}

static void tick_mem_write(mem_write_state_t &s) {
  if (s.call.valid) {
    s.call.valid = false;
    s.ret.valid = true;
    s.ret.live = s.call.live;
    mem_state[s.call.arg0] = s.call.arg1;
    std::cout << "MEM STORE: " << s.call.arg1 << " @" << s.call.arg0 << std::endl;
  }
}

// mem_read declarations
struct mem_read_call_t  { bool valid; void *live; ui<26> arg0; unsigned cyc; };
struct mem_read_ret_t   { bool valid; void *live; array<16, ui<32> > rval; };
struct mem_read_state_t {
  mem_read_call_t call;
  mem_read_ret_t ret;

  std::vector<mem_read_call_t> reqs;
};

static void init_mem_read(mem_read_state_t &s) {
  s.call.valid = false;
  s.ret.valid = false;
}

static void tick_mem_read(mem_read_state_t &s) {
  const unsigned READ_CYC = 1000;
  
  if (s.call.valid) {
    s.call.valid = false;
    s.call.cyc = READ_CYC;

    s.reqs.push_back(s.call);
  }

  for (auto &r : s.reqs) {
    if (!r.cyc--) {
      s.ret.valid = true;
      s.ret.live = r.live;
      s.ret.rval = mem_state[r.arg0];

      std::cout << "MEM LOAD: " << s.ret.rval << " @" << r.arg0 << std::endl;
    }
  }
}

// print_hex declarations.
struct print_hex_state_t;
static void init_print_hex(print_hex_state_t&);
static void tick_print_hex(print_hex_state_t&);
static void tick_mem_read(mem_read_state_t&);

struct print_hex_call_t    { bool valid; void *live;  ui<32> arg0; };
struct print_hex_ret_t     { bool valid; void *live;               };
struct print_hex_bb0_in_t  { bool valid; void *live;  ui<32> arg0; };
struct print_hex_bb0_out_t { bool valid; void *live;               };

struct print_hex_state_t {
  print_hex_call_t call;
  print_hex_ret_t ret;
  print_hex_bb0_in_t bb0_in;
  print_hex_bb0_out_t bb0_out;
};

static void init_print_hex(print_hex_state_t &s) {
  s.bb0_in.valid = false;
  s.bb0_out.valid = false;
}

static void tick_print_hex(print_hex_state_t &s) {
  // arbiter for basic block0
  // print_hex call input.
  if (!s.bb0_in.valid && s.call.valid) {
    s.call.valid = false;
    s.bb0_in.valid = true;
    s.bb0_in.live = s.call.live;
    s.bb0_in.arg0 = s.call.arg0;
  }

  // basic block 0
  if (s.bb0_in.valid && !s.bb0_out.valid) {
    using namespace std;

    ui<32>  val0;
    val0 = s.bb0_in.arg0;
    cout << "printhex> " << hex << setw(8) << setfill('0') << val0
         << dec << endl;

    s.ret.valid = true;
    s.ret.live = s.bb0_in.live;
    s.bb0_out.valid = false;

    // output connections
    if (!s.bb0_out.valid && s.bb0_in.valid) {
      s.bb0_in.valid = false;
      s.bb0_out.live = s.bb0_in.live;
    }
  }
}

struct print_hex2_state_t;
static void init_print_hex2(print_hex2_state_t&);
static void tick_print_hex2(print_hex2_state_t&);

struct print_hex2_call_t    { bool valid; void *live;  ui<32> arg0;  ui<32> arg1; };
struct print_hex2_ret_t     { bool valid; void *live;               };
struct print_hex2_bb0_in_t  { bool valid; void *live;  ui<32> arg0;  ui<32> arg1; };
struct print_hex2_bb0_out_t { bool valid; void *live;               };

struct print_hex2_state_t {
  print_hex2_call_t call;
  print_hex2_ret_t ret;
  print_hex2_bb0_in_t bb0_in;
  print_hex2_bb0_out_t bb0_out;
};

static void init_print_hex2(print_hex2_state_t &s) {
  s.bb0_in.valid = false;
  s.bb0_out.valid = false;
}

static void tick_print_hex2(print_hex2_state_t &s) {
  // arbiter for basic block0
  // print_hex call input.
  if (!s.bb0_in.valid && s.call.valid) {
    s.call.valid = false;
    s.bb0_in.valid = true;
    s.bb0_in.live = s.call.live;
    s.bb0_in.arg0 = s.call.arg0;
    s.bb0_in.arg1 = s.call.arg1;
  }

  // basic block 0
  if (s.bb0_in.valid && !s.bb0_out.valid) {
    using namespace std;

    ui<32>  val0, val1;
    val0 = s.bb0_in.arg0;
    val1 = s.bb0_in.arg1;
    cout << "printhex> " << hex << setw(8) << setfill('0') << val0 << ' ' << setw(8) << setfill('0') << val1 << dec << endl;

    s.ret.valid = true;
    s.ret.live = s.bb0_in.live;
    s.bb0_out.valid = false;

    // output connections
    if (!s.bb0_out.valid && s.bb0_in.valid) {
      s.bb0_in.valid = false;
      s.bb0_out.live = s.bb0_in.live;
    }
  }
}

template <typename T> T st_idx(T in, int i, int sz, unsigned x) {
  unsigned long mask(((1<<sz)-1)<<i);

  return (in & ~mask) | ((x<<i)&mask);
}

template <typename T> T st_idx(T in, int i, bool x) {
  unsigned long mask(1ul<<i);

  return (in & ~mask) | ((x?1:0)<<i);
}

static void init_mem_state() {
  const unsigned N = 65536;
  for (unsigned i = 0; i < N; i++)
    mem_state[i/16][i%16] = i;
}

}
}

#endif
