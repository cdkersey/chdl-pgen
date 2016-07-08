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

  if (f) assemble_func();
  
  f = &p.functions[name];
  func_name = name;
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
  arg_ids[v].push_back(id);

  return *this;
}

asm_prog &bscotch::asm_prog::const_arg(long const_arg) {
  to_vec_bool(v->const_val, v->t.size(), const_arg);

  return *this;
}

asm_prog &bscotch::asm_prog::static_arg(std::string static_name) {
  v->static_arg = &f->static_vars[static_name];

  return *this;
}

asm_prog &bscotch::asm_prog::func_arg(std::string func_name) {
  v->func_arg = func_name;

  return *this;
}

asm_prog &bscotch::asm_prog::br(asm_prog::val_id_t sel) {
  br_id[b] = sel;

  return *this;
}

asm_prog &bscotch::asm_prog::br() {
  b->branch_pred = NULL;

  return *this;
}

asm_prog &bscotch::asm_prog::target(std::string label) {
  br_targets[b].push_back(label);

  return *this;
}

// Fill in basic block successor/predecessor information for most
// recent function.
void bscotch::asm_prog::bb_resolveptrs() {
  // Find successors.
  for (auto &b : br_targets) {
    for (unsigned i = 0; i < b.second.size(); ++i) {
      if (labels.count(b.second[i])) {
        b.first->suc.push_back(labels[b.second[i]]);
      } else {
        cout << "Unresolved label \"" << b.second[i]
             << "\" in function \"" << func_name << "\"." << endl;
        abort();
      }
    }
  }

  // Use successors to find predecessors.
  for (auto &b : f->bbs)
    for (auto &s : b->suc)
      s->pred.push_back(b);
}

void bscotch::asm_prog::id_liveness() {
  // Reverse id_to_val to get ids when provided with val pointers.
  map<if_val*, val_id_t> vp_to_id;
  for (auto &p : id_to_val)
    for (auto &vp : p.second)
      vp_to_id[vp] = p.first;

  // Get defs and uses.
  map<if_bb*, set<val_id_t> > def, use;
  for (auto &b : f->bbs) {
    for (auto &v : b->vals) {
      def[b].insert(vp_to_id[v]);
      for (auto &a : arg_ids[v])
        use[b].insert(a);
    }
  }

  // Find initial live_out and live_in based on defs/uses. TODO
  map<if_bb*, set<val_id_t> > live_in, live_out;
  
  // Iterate until live_out and live_in do not change; these are live sets. TODO
}

void bscotch::asm_prog::assemble_func() {
  bb_resolveptrs();
  
  // Do liveness analysis in terms of val ids.
  id_liveness();

  // Add phis where appropriate. TODO

  // Apply unique (SSA) value ids.

  // Fill in args and branch predicates with pointers. TODO

  // Fill in liveness information in basic blocks. TODO

  // Prevent repeat assembly.
  f = NULL;
}
