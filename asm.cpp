#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#include "type.h"
#include "if.h"
#include "asm.h"
#include "break_cycles.h"
#include "prevent_deadlock.h"

using namespace std;
using namespace bscotch;

void bscotch::asm_prog::function(string name) {
  if (f) assemble_func();
  labels.clear();
  id_to_val.clear();
  arg_ids.clear();
  predicate.clear();
  br_id.clear();
  stall_id.clear();
  br_targets.clear();
  
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
  cout << "New val with type ";
  type u(t);
  print(cout, u);
  cout << endl;
  
  // If this is an argument, add it to function argument types.
  // (TODO: only in first basic block)
  if (op == VAL_ARG) f->args.push_back(t);
  
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

asm_prog &bscotch::asm_prog::stall(asm_prog::val_id_t in) {
  stall_id[b] = in;

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

static void ssa_liveness_analysis(if_func *f) {
  // Find gen_v, gen_id, and kill sets used by liveness analysis.
  map<if_bb*, set<if_val*> > kill, gen, live_in, live_out;
  map<if_bb*, map<if_bb*, set<if_val*> > > live_in_mask;

  for (auto &b : f->bbs) {
    set<if_val*> def_so_far;
    for (auto &v : b->vals) {
      if (v->pred && !def_so_far.count(v->pred))
        gen[b].insert(v->pred);
      for (auto &a : v->args)
        if (!def_so_far.count(a))
          gen[b].insert(a);

      // Mask out all off-path phi arguments.
      if (v->op == VAL_PHI)
        for (unsigned i = 0; i < b->pred.size(); ++i)
          for (unsigned j = 0; j < b->pred.size(); ++j)
            if (v->args[j] != v->args[i])
              live_in_mask[b->pred[i]][b].insert(v->args[j]);
 
      kill[b].insert(v);
      def_so_far.insert(v);
    }
    
    if (b->branch_pred && !def_so_far.count(b->branch_pred))
      gen[b].insert(b->branch_pred);

    // The following will only occur in code that may
    // intentionally stall forever:
    if (b->stall && !def_so_far.count(b->stall))
      gen[b].insert(b->stall);
  }

  // Do liveness analysis.
  bool changed;
  do {
    changed = false;

    for (auto &b : f->bbs) {
      set<if_val*> new_live_in, new_live_out;
      new_live_in = gen[b];
      for (auto &v : live_out[b])
        if (!kill[b].count(v))
          new_live_in.insert(v);

      if (new_live_in != live_in[b]) {
        changed = true;
        live_in[b] = new_live_in;
      }

      for (auto &s : b->suc)
        for (auto &v : live_in[s])
          if (!live_in_mask[b][s].count(v))
           new_live_out.insert(v);

      if (new_live_out != live_out[b]) {
        changed = true;
        live_out[b] = new_live_out;
      }
    }
  } while (changed);

  for (auto &b : f->bbs) {
    for (auto &v : live_in[b]) b->live_in.push_back(v);
    for (auto &v : live_out[b]) b->live_out.push_back(v);
  }
}

void bscotch::asm_prog::assemble_func() {
  // Reverse id_to_val for quick lookup.
  map<if_val*, val_id_t> val_to_id;
  for (auto &x : id_to_val)
    for (auto &vp : x.second)
      val_to_id[vp] = x.first;

  // Assign the basic blocks and values sequential IDs.
  int id = 0;
  for (unsigned i = 0; i < f->bbs.size(); ++i) {
    f->bbs[i]->id = i;
    for (auto &v : f->bbs[i]->vals) {
      v->id = id++;
      v->args.resize(arg_ids[v].size());
    }
  }

  // Set "suc" and "pred" pointers in basic blocks.
  bb_resolveptrs();
  
  // Find gen_v, gen_id, and kill sets used by liveness analysis.
  map<if_bb*, set<val_id_t> > kill, gen, live_in, live_out;

  for (auto &b : f->bbs) {
    set<val_id_t> def_so_far;
    for (auto &v : b->vals) {
      if (predicate.count(v) &&!def_so_far.count(predicate[v]))
        gen[b].insert(predicate[v]);
      for (auto &a : arg_ids[v])
        if (!def_so_far.count(a))
          gen[b].insert(a);

      kill[b].insert(val_to_id[v]);
      def_so_far.insert(val_to_id[v]);
    }
    
    if (br_id.count(b) && !def_so_far.count(br_id[b]))
      gen[b].insert(br_id[b]);

    // The following will only occur in code that may
    // intentionally stall forever:
    if (stall_id.count(b) && !def_so_far.count(stall_id[b]))
      gen[b].insert(stall_id[b]);
  }

  // Do ID-level liveness analysis.
  bool changed;
  do {
    changed = false;

    for (auto &b : f->bbs) {
      set<val_id_t> new_live_in, new_live_out;
      new_live_in = gen[b];
      for (auto &v : live_out[b])
        if (!kill[b].count(v))
          new_live_in.insert(v);

      if (new_live_in != live_in[b]) {
        changed = true;
        live_in[b] = new_live_in;
      }

      for (auto &p : b->suc)
        for (auto &v : live_in[p])
          new_live_out.insert(v);

      if (new_live_out != live_out[b]) {
        changed = true;
        live_out[b] = new_live_out;
      }
    }
  } while (changed);

  for (auto &b : f->bbs) {
    cout << "bb " << b->id << ", kill:";
    for (auto &x : kill[b]) cout << ' ' << x;
    cout << ", gen:";
    for (auto &x : gen[b]) cout << ' ' << x;
    cout << ", live_in:";
    for (auto &x : live_in[b]) cout << ' ' << x;
    cout << ", live_out:";
    for (auto &x : live_out[b]) cout << ' ' << x;
    cout << endl;
  }

  // Find which versions of variables are live out of which basic blocks.
  for (auto &x : id_to_val) {
    cout << "Variable ID " << x.first << endl;

    bool changed, phi_added;
    map<if_bb*, if_val*> phi_set;
    do {
      set<if_bb*> live_in_bbs, live_out_bbs;
      map<if_bb*, if_val*> wr_ver;
      for (auto &b : f->bbs) {
        if (live_out[b].count(x.first)) live_out_bbs.insert(b);
        if (live_in[b].count(x.first)) live_in_bbs.insert(b);
 
        for (auto &v : b->vals)
          if (val_to_id[v] == x.first)
            wr_ver[b] = v;
      }
      
      bool change;
      map<if_bb*, set<if_val*> > in_vers, out_vers;
      do {
        change = false;
 
        for (auto &b : f->bbs) {
          set<if_val*> new_in_vers, new_out_vers;

          for (auto &p : b->pred)
            if (live_out_bbs.count(p))
              for (auto &v : out_vers[p])
                new_in_vers.insert(v);
          
          if (wr_ver.count(b)) new_out_vers.insert(wr_ver[b]);
          else new_out_vers = new_in_vers;
          
          if (new_in_vers != in_vers[b]) {
            change = true;
            in_vers[b] = new_in_vers;
          }

          if (new_out_vers != out_vers[b]) {
            change = true;
            out_vers[b] = new_out_vers;
          }
        }
      } while (change);

      for (auto &b : f->bbs) {
        cout << "block " << b->id << " in vers:";
        for (auto &v : in_vers[b])        
          cout << ' ' << v->id;
        cout << endl;
        cout << "block " << b->id << " out vers:";
        for (auto &v : out_vers[b])
          cout << ' ' << v->id;
        cout << endl;
      }

      phi_added = false;
 
      // Identify convergence points (phi candidate points)
      for (auto &b : f->bbs) {
        bool convergence(b->pred.size() > 1 && in_vers[b].size() > 1 &&
                         !phi_set.count(b));

        if (convergence) cout << "Convergence candidate at BB " << b->id << endl;
        
        if (convergence)
          for (auto &p : b->pred)
            if (p != b && out_vers[p].size() != 1) {
              convergence = false;
              cout << " but predecessor " << p->id << " has " << out_vers[p].size() << " out versions." << endl;
            }
         
        // Add phis at convergence points, then update vers. Do this until
        // no blocks have multiple versions of the variable.
        if (convergence) {
          cout << "Convergence point at BB " << b->id << endl;
          if_val *phi = new if_val();
          phi_set[b] = phi;
          phi->op = VAL_PHI;
          phi->t = (*in_vers[b].begin())->t;
          phi->id = id++;
          phi->bb = b;
          for (auto &p : b->pred) {
            if (p == b)
              if (wr_ver.count(b))
                phi->args.push_back(wr_ver[b]);
              else
                phi->args.push_back(phi);
            else
              for (auto &v : out_vers[p])
                phi->args.push_back(v);
          }
          b->vals.insert(b->vals.begin(), phi);
          id_to_val[x.first].insert(phi);
          val_to_id[phi] = x.first;
          // No args: should not have to 
          
          phi_added = true;
        }
      }

      // Now that we know which version to use, assign args and set
      // live_out accordingly.
      for (auto &b : f->bbs) {
        cout << "ver[" << b->id << "] has " << in_vers[b].size() << " entries:";
        for (auto &p : in_vers[b]) cout << ' ' << p->id;
        cout << endl;
        cout << "in block:";
        for (auto &p : b->vals)
          if (val_to_id[p] == x.first) cout << ' ' << p->id;
        cout << endl;
 
        if_val *current_ver;
        if (phi_set.count(b))
          current_ver = phi_set[b];
        else
          current_ver = *(in_vers[b].begin());
        for (auto &v : b->vals) {
          if (predicate.count(v) && predicate[v] == x.first)
            v->pred = current_ver;
          for (unsigned i = 0; i < arg_ids[v].size(); ++i)
            if (arg_ids[v][i] == x.first)
              v->args[i] = current_ver;
          if (val_to_id[v] == x.first) current_ver = v;
        }
        if (br_id.count(b) && br_id[b] == x.first)
          b->branch_pred = current_ver;
        if (stall_id.count(b) && stall_id[b] == x.first)
          b->stall = current_ver;
      }
    } while (phi_added);
  }

  // Do SSA-mode liveness analysis
  ssa_liveness_analysis(f);

  // Flag back edges for 2-entry pipeline buffers.
  break_cycles(*f);

  // Re-order block predecessor priorities to avoid deadlocks.
  prevent_deadlock(*f);
  
  // Prevent repeat assembly.
  f = NULL;
}
