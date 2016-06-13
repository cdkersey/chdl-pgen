#ifndef CHDL_LATE_H
#define CHDL_LATE_H

namespace chdl {

template <typename T> struct late : public T {
  late() {}
  late(const T &x): T(x) {}
  
  late &operator=(const T &r) { T(*this) = r; return *this; }
  late &operator=(const late<T> &r) { T(*this) = T(r); return *this; }

  operator T() const { return *this; }
  operator T&() const { return *this; }
  operator const T&() const { return *this; }
};

template <typename T> struct sz<late<T> > {
  static const unsigned value = sz<T>::value;
};

ag_endtype Wreg(node wr, const ag_endtype &et) { return ag_endtype(); }
ag_endtype Reg(const ag_endtype &et) { return ag_endtype(); }
  
template <typename NAME, typename T, typename NEXT>
  ag<NAME, T, NEXT> Wreg(node wr, ag<NAME, T, NEXT> &d)
{
  ag<NAME, T, NEXT> r;
  r.contents = Wreg(wr, d.contents);
  r.next = Wreg(wr, d.next);

  return r;
}

template <typename NAME, typename T, typename NEXT>
  ag<NAME, T, NEXT> Reg(ag<NAME, T, NEXT> &d)
{
  ag<NAME, T, NEXT> r;
  r.contents = Reg(d.contents);
  r.next = Reg(d.net);
  return r;
}
  
template <typename T> late<T> Wreg(node wr, const late<T> &x) {
  return Mux(Reg(wr), Wreg(Reg(wr), T(x)), T(x));
}

template<typename T> void tap(std::string name, const late<T> &x) {
  tap(name, T(x));  
}

}

#endif
