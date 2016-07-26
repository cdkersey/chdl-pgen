#include <iostream>
#include <vector>
#include <cstring>

template <unsigned N> using ui = unsigned;
template <unsigned N> using si = int;
template <unsigned N, typename T> struct array {
  array(): contents(N) {}

  std::vector<T> contents;
};

template <typename T> struct concatenator {
  concatenator(T &out): out(out), bit_pos(0) {}
  
  concatenator operator()(const T &x) {
    out = x;
    return *this;
  }

  template <typename U> concatenator operator()(const U &x) {
    std::cerr << "Actual concatenation not implemented." << std::endl;
    abort();
    return *this;
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
  memset(&s, 0, sizeof(s));
  s.call.valid = true;
  
  for (unsigned i = 0; i < 10000; ++i) {
    tick_bmain(s);
  }
  
  return 0;
}
