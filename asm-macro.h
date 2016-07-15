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

  void function(const char *name);
  void label(const char *name);
  void br(const char *dest);
  argcollector<std::string> br(var &sel);
  argcollector<var> spawn(const char *func);

  void call();
  void ret();
  void ret(var &rval);
}

#endif
