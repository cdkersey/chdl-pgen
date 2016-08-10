#ifndef CHDL_PGEN_ASM_MACRO_H
#define CHDL_PGEN_ASM_MACRO_H

#include <iostream>

#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <functional>

#include "asm.h"
#include "type.h"

namespace pgen {
  // Initialize the macro API. This is a thread-unsafe, stateful API designed as
  // a stop-gap solution between the assembler and a language front-end/parser.
  void init_macro_env(asm_prog &a);

  void finish_macro_env();
  
  struct varimpl;
  
  struct var {
    var(const type &t);
    var();

    var &operator=(const var &r); // Assignment, propagates type inference.

    varimpl *p;
  };

  var lit(const type &t, unsigned long val);
  var arg(const type &t);
  
  // Basic arithmetic/logic operators
  var operator&(const var &a, const var &b);
  var operator|(const var &a, const var &b);
  var operator^(const var &a, const var &b);
  var operator+(const var &a, const var &b);
  var operator-(const var &a, const var &b);
  var operator*(const var &a, const var &b);
  var operator/(const var &a, const var &b);

  // Basic unary operators
  var operator!(const var &x);
  var operator-(const var &x);
  var operator~(const var &x);

  var OrReduce(const var &x);
  var AndReduce(const var &x);
  
  // Basic comparison operators
  var operator==(const var &a, const var &b);
  var operator!=(const var &a, const var &b);
  var operator<(const var &a, const var &b);
  var operator>=(const var &a, const var &b);
  var operator>(const var &a, const var &b);
  var operator<=(const var &a, const var &b);

  template <typename T> struct argcollector {
    argcollector(std::function<void(T)> f): f(f) {}
    argcollector &operator()(const T& x) { f(x); return *this; }
    std::function<void(T)> f;
  };
  
  void static_var(const char *name, const type &t);
  void static_var(const char *name, const type &t, long initialval);

  var load(const char *name);
  var load(const char *name, const var &idx);
  var load(const var &in, const var &idx);
  var load(const var &in, const var &idx, const var &len);
  var load(const var &in, const var &idx, unsigned len);
  var load(const var &in, const char *field);

  void store(const char *name, const var &d);
  void store(const char *name, const var &idx, const var &d);
  void store(const char *name, const char *field, const var &d);
  var repl(const var &in, const var &idx, const var &d);
  var repl(const var &in, const var &idx, const var &d, const var &len);
  var repl(const var &in, const var &idx, const var &d, unsigned len);
  var repl(const var &in, const char *field, const var &d);

  // Add a predicate to preceding store to a static variable.
  void pred(const var &p);
  
  void function(const char *name);
  void label(const char *name);
  void br(const char *dest);
  argcollector<std::string> br(const var &sel);
  argcollector<var> spawn(const char *func);
  argcollector<var> call(const char *func);
  argcollector<var> call(const char *func, const var &rval);
  argcollector<var> cat(const var &r);
  argcollector<var> build(const var &r);
  
  void ret();
  void ret(const var &rval);
}

#endif
