#include "asm-macro.h"

#include <cstdlib>

using namespace bscotch;
using namespace std;

// Initialize the macro API. This is a thread-unsafe, stateful API designed as
// a stop-gap solution between the assembler and a language front-end/parser.
void bscotch::init_macro_env(asm_prog &a) {
}
  
bscotch::var::var(const type &t): t(t) {
}

var &bscotch::var::operator=(const var &r) {
}

// Basic arithmetic/logic operators
var bscotch::operator&(const var &a, const var &b) {
}

var bscotch::operator|(const var &a, const var &b) {
}

var bscotch::operator^(const var &a, const var &b) {
}

var bscotch::operator+(const var &a, const var &b) {
}

var bscotch::operator-(const var &a, const var &b) {
}

var bscotch::operator*(const var &a, const var &b) {
}

var bscotch::operator/(const var &a, const var &b) {
}

// Basic comparison operators
var bscotch::operator==(const var &a, const var &b) {
}

var bscotch::operator!=(const var &a, const var &b) {
}

var bscotch::operator<(const var &a, const var &b) {
}

var bscotch::operator>=(const var &a, const var &b) {
}

var bscotch::operator>(const var &a, const var &b) {
}

var bscotch::operator<=(const var &a, const var &b) {
}

// Basic unary operators
var operator-(const var &x) {
}

var operator~(const var &x) {
}

// Custom literal type. 1234_c is a var whose type will be inferred.
var bscotch::operator "" _c(const char *x) {
  long long val = strtoll(x, NULL, 0);
}

void bscotch::function(const char *name) {
}

void bscotch::label(const char *name) {
}

void bscotch::br(const char *dest) {
}

argcollector<const char *> bscotch::br(var &sel) {
}

argcollector<var> bscotch::spawn(const char *func) {
}

void bscotch::call() {
}

void bscotch::ret() {
}

void bscotch::ret(var &rval) {
}
