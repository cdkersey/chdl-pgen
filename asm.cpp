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

  if (b && !br_targets[b].size()) br_targets[b].push_back(name);
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

void bscotch::asm_prog::val_liveness() {
  #if 0
  // Reverse id_to_val to get ids when provided with val pointers. (TODO: SSA)
  map<if_val*, val_id_t> vp_to_id;
  for (auto &p : id_to_val)
    for (auto &vp : p.second)
      vp_to_id[vp] = p.first;

  // Get defs and uses. (TODO: SSA)
  map<if_bb*, set<val_id_t> > def, use, def_after_use;
  for (auto &b : f->bbs) {
    for (auto &v : b->vals) {
      if (!def[b].count(vp_to_id[v]) && use[b].count(vp_to_id[v]))
	def_after_use[b].insert(vp_to_id[v]);
      def[b].insert(vp_to_id[v]);
      for (auto &a : arg_ids[v])
        use[b].insert(a);
    }
  }

  // Find initial live_out and live_in based on defs/uses. (TODO: SSA)
  map<if_bb*, set<val_id_t> > live_in, live_out;
  for (auto &x : use)
    for (auto &id : x.second)
      if (!def[x.first].count(id) || def_after_use[x.first].count(id))
	live_in[x.first].insert(id);

  for (auto &x : def)
    for (auto &id : x.second)
      if (!use[x.first].count(id) || def_after_use[x.first].count(id))
	live_out[x.first].insert(id);
  
  // Iterate until live_out and live_in do not change. (TODO: SSA)
  bool changed;
  do {
    changed = false;
    for (auto &b : f->bbs) {
      // live_out = (def | live_in) & OrN(suc.live_in); (SSA)
      // live_in = (live_out | use) & ~def; (SSA)
    }
  } while (changed);
  #endif
}

template <typename T>
  static void set_union(set<T> &o, const set<T> &a, const set<T> &b)
{
  for (auto &x : a) o.insert(x);
  for (auto &x : b) o.insert(x);
}

template <typename T>
  static void set_intersect(set<T> &o, const set<T> &a, const set<T> &b)
{
  set<T> out;
  for (auto &x : a)
    if (b.count(x)) out.insert(x);
  o = out;
}

template <typename T>
  static bool set_eq(const set<T> &a, const set<T> &b)
{
  if (a.size() != b.size()) return false;

  for (auto &x : a)
    if (!b.count(x)) return false;

  return true;
}

void bscotch::asm_prog::dom_analysis(map<if_bb*, set<if_bb*> > &dominates) {
  map<if_bb*, set<if_bb*> > d_by, d_front;
  for (auto &b : f->bbs)
    d_by[b].insert(b);

  for (auto &b : f->bbs) {
    bool changed;

    d_by.clear();
    for (auto &x : f->bbs) {
      if (x == b) {
	d_by[x].insert(x);
      } else {
	for (auto &y : f->bbs)
	  d_by[x].insert(y);
      }
    }

    do {
      changed = false;

      for (auto &c : f->bbs) {
        set<if_bb*> new_d_by;
        for (auto &x : f->bbs) new_d_by.insert(x);
        bool empty = true;
        for (auto it = c->pred.begin(); it != c->pred.end(); ++it)
          if (*it != c) {
            set_intersect(new_d_by, new_d_by, d_by[*it]);
            empty = false;
          }
        if (empty) new_d_by.clear();

        new_d_by.insert(c);
      
        if (!set_eq(d_by[c], new_d_by)) {
          changed = true;
          d_by[c] = new_d_by;
        }
      }
    } while (changed);

    for (auto &x : d_by[b])
      dominates[x].insert(b);
  }

  for (auto &x : dominates)
    for (auto &y : x.second)
      cout << "bb " << x.first->id << " dominates bb " << y->id << endl;
}

// Find all defs with id "id" that reach bb "use_bb", index "idx"
void bscotch::asm_prog::reaches
  (set<if_val*> &defs, if_bb *use_bb, int idx, val_id_t id, set<if_bb*> &vis)
{
  // If we have visited this (whole) basic block before, do not re-scan it.
  if (vis.count(use_bb)) return;
  else if (idx == use_bb->vals.size()) vis.insert(use_bb);
  
  // Scan the basic block backwards for the value id.
  for (--idx; idx >= 0; --idx) {
    if (id_to_val[id].count(use_bb->vals[idx])) {
      defs.insert(use_bb->vals[idx]);
      return;
    }
  }

  // If execution reaches here, we did not find a reaching definition in this
  // block. Recurse.
  for (auto &p : use_bb->pred)
    reaches(defs, p, p->vals.size(), id, vis);
}

void bscotch::asm_prog::val_resolveptrs(int phi_id) {
  for (auto &b : f->bbs) {
    map<set<if_val*>, if_val*> phis;
  
    for (unsigned i = 0; i < b->vals.size(); ++i) {
      for (unsigned j = 0; j < arg_ids[b->vals[i]].size(); ++j) {
        set<if_val *> s;
	set<if_bb *> visited;
        reaches(s, b, i, arg_ids[b->vals[i]][j], visited);

	
	cout << "Defs reaching BB " << b->id << " val " << i << " arg "
             << j << ':';
	for (auto &x : s)
	  cout << ' ' << x->id;
	cout << endl;

	if (s.size() == 1) {
	  // Set arg to pointer.
	  b->vals[i]->args.push_back(*s.begin());
	} else {
	  // Create phi if necessary.
	  if (!phis.count(s)) {
	    if_val *phi = phis[s] = new if_val();
	    phi->op = VAL_PHI;
	    phi->t = (*s.begin())->t;
	    phi->id = phi_id++;
	    phi->bb = b;
	    for (auto &x : s)
              phi->args.push_back(x);
	    b->vals.insert(b->vals.begin(), phi);
	  }
	  
	  // Set arg to phi.
	  b->vals[i]->args.push_back(phis[s]);
	}
      }
    }
  }  
}

void bscotch::asm_prog::assemble_func() {
  // Assign the basic blocks sequential IDs.
  for (unsigned i = 0; i < f->bbs.size(); ++i) f->bbs[i]->id = i;

  // Assign initial, temporary IDs to values.
  int id = 0;
  for (auto &b : f->bbs)
    for (auto &v : b->vals)
      v->id = id++;
  
  // Set "suc" and "pred" pointers in basic blocks.
  bb_resolveptrs();

  // Add phis as needed and resolve args.
  val_resolveptrs(id);
  
  // Perform dominator analysis and find dominance frontiers.
  map<if_bb*, set<if_bb*> > dominates;
  dom_analysis(dominates);
  
  // Find all defs reaching each arg of each val

  // Apply unique (SSA) value ids.
  
  // Do liveness analysis in terms of val pointers. (find live_in, live_out)
  // val_liveness();

  // Fill in args and branch predicates with pointers. TODO

  // Fill in liveness information in basic blocks. TODO

  // Prevent repeat assembly.
  f = NULL;
}
