#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#include "type.h"
#include "if.h"
#include "asm.h"

using namespace std;
using namespace bscotch;

void bscotch::asm_prog::function(string name) {
  labels.clear();
  
  f = &p.functions[name];
}

void bscotch::asm_prog::label(string name) {
  if_bb *bb = new if_bb();

  labels[name] = bb;

  f->bbs.push_back(bb);
}

asm_prog &bscotch::asm_prog::val(type &t, asm_prog::val_id_t id, if_op op) {
  // TODO

  return *this;
}

asm_prog &bscotch::asm_prog::val(asm_prog::val_id_t id, if_op op) {
  // TODO

  return *this;
}

asm_prog &bscotch::asm_prog::arg(asm_prog::val_id_t id) {
  // TODO

  return *this;
}

asm_prog &bscotch::asm_prog::arg(long const_arg) {
  // TODO

  return *this;
}

asm_prog &bscotch::asm_prog::arg(std::string static_name) {
  // TODO

  return *this;
}

asm_prog &bscotch::asm_prog::br(asm_prog::val_id_t sel) {
  // TODO

  return *this;
}

asm_prog &bscotch::asm_prog::br() {
  // TODO

  return *this;
}

asm_prog &bscotch::asm_prog::target(std::string label) {
  // TODO

  return *this;
}
