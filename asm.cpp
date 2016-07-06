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

  b = bb;
  
  labels[name] = bb;

  f->bbs.push_back(bb);
}

void bscotch::asm_prog::static_var(const type &t, string name) {
  f->static_vars[name].name = name;
  f->static_vars[name].t = t;
  f->static_vars[name].broadcast = false;
}

void bscotch::asm_prog::bcast_var(const type &t, string name) {
  static_var(t, name);
  f->static_vars[name].broadcast = true;
}

asm_prog &bscotch::asm_prog::val(const type &t, asm_prog::val_id_t id, if_op op)
{
  if_val *vp = new if_val();

  v = vp;

  b->vals.push_back(vp);

  vp->t = t;
  vp->op = op;

  id_to_val[id].insert(vp);
  
  return *this;
}

asm_prog &bscotch::asm_prog::val(asm_prog::val_id_t id, if_op op) {
  val(void_type(), id, op);

  return *this;
}

asm_prog &bscotch::asm_prog::arg(asm_prog::val_id_t id) {
  // TODO

  return *this;
}

asm_prog &bscotch::asm_prog::const_arg(long const_arg) {
  // TODO

  return *this;
}

asm_prog &bscotch::asm_prog::static_arg(std::string static_name) {
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
