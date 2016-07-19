#ifndef BSCOTCH_ASM_MACRO_H
#define BSCOTCH_ASM_MACRO_H

#include <iostream>

#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>

#include "asm.h"
#include "type.h"

namespace bscotch {
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
    argcollector(std::vector<T> &v): v(v) {}
    
    argcollector &operator()(const T& x) {
      v.push_back(x);
      return *this;
    }

    std::vector<T> &v;
  };
  
  // Custom literal type. 1234_c is a var whose type will be inferred.
  var operator "" _c(const char *x);

  void static_var(const char *name, const type &t);
  void static_var(const char *name, const type &t, long initialval);

  var load(const char *name);
  var load(const char *name, const var &idx);
  var load(const var &in, const var &idx);
  var load(const var &in, const var &idx, long len);
  var load(const var &in, const char *field);

  void store(const char *name, const var &d);
  void store(const char *name, const var &idx, const var &d);
  void store(const char *name, const char *field, const var &d);
  var repl(const var &in, const var &idx, const var &d);
  var repl(const var &in, const var &idx, const var &d, unsigned len);
  var repl(const var &in, const char *field, const var &d);
  var repl(const var &in, const char *field, const var &d, unsigned len);
  
  void function(const char *name);
  void label(const char *name);
  void br(const char *dest);
  argcollector<std::string> br(const var &sel);
  argcollector<var> spawn(const char *func);

  void call();
  void ret();
  void ret(const var &rval);
}

#endif
