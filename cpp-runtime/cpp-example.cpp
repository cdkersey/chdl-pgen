#include <iostream>
#include <vector>
#include <cstring>

template <unsigned N> struct ui {
  ui(): val(0) {}
  ui(unsigned long val): val(val) {}
  operator unsigned long() const { return val; }
  ui &operator=(unsigned long x) { val = x; return *this; }
  unsigned long val;
};

template <unsigned N> struct si {
  si(): val(0) {}
  si(long val): val(val) {}
  operator long() const { return val; return *this; }
  si &operator=(long x) { val = x; }
  long val;
};
  
template <unsigned N, typename T> struct array {
  array(): contents(N) {}

  std::vector<T> contents;
};

bool or_reduce(const ui<0> &x) { return false; }
bool and_reduce(const ui<0> &x) { return true; }

bool or_reduce(const ui<1> &x) { return x&1; }
bool and_reduce(const ui<1> &x) { return x&1; }

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
  concatenator(T &out): out(out), bit_pos(0) {}
  
  concatenator operator()(const T &x) {
    out = x;
    return *this;
  }

  template <unsigned N> concatenator operator()(const ui<N> &x) {
    out |= (x & ((1<<N)-1)) << bit_pos;
    bit_pos += N;
  }
  
  T &out;
  unsigned bit_pos;
};

template <typename T> concatenator<T> cat(T &out) {
  return concatenator<T>(out);
}

#include "./cgen-out.incl"

int main() {
  bmain_state_t s;
  init_bmain(s);
  s.call.valid = true;
  
  for (unsigned i = 0; i < 10000; ++i) {
    tick_bmain(s);
  }
  
  return 0;
}
