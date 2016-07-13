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
    
    if (b->branch_pred && !def_so_far.count(val_to_id[b->branch_pred]))
      gen[b].insert(val_to_id[b->branch_pred]);
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
    do {      
      // Which versions of the value does each basic block carry?
      set<if_bb*> wr_only;
      map<if_bb*, if_val*> wr_ver;
      map<if_bb*, set<if_val*> > ver;

      // Initial set; every block that writes the variable has the last write as
      // its output version.
      for (auto &b : f->bbs) {
        bool read = false;

        for (auto &v : b->vals) {
          for (auto &a : arg_ids[v])
            if (a == x.first) read = true;
          if (predicate.count(v) && predicate[v] == x.first) read = true;
          if (val_to_id[v] == x.first) {
            wr_ver[b] = v;
            if (!read) wr_only.insert(b);
          }
        }
      }

      cout << "wr_only bbs:";
      for (auto &b : wr_only) cout << ' ' << b->id;
      cout << endl;

      do {
        changed = false;

        for (auto &b : f->bbs) {
          set<if_val*> new_ver;
          if (wr_only.count(b)) {
            new_ver.insert(wr_ver[b]);
          } else if (live_in[b].count(x.first)) {
            if (wr_ver.count(b))
              new_ver.insert(wr_ver[b]);
            
            for (auto &p : b->pred)
              for (auto &v : ver[p])
                new_ver.insert(v);
          }

          if (ver[b] != new_ver) {
            changed = true;
            ver[b] = new_ver;
          }
        }
      } while (changed);


      phi_added = false;
 
      // Identify convergence points (phi candidate points)
      for (auto &b : f->bbs) {
        bool convergence(b->pred.size() > 1 &&
                         !wr_only.count(b) && ver[b].size() > 1);
        if (convergence)
          for (auto &p : b->pred)
            if (ver[p].size() != 1 && !wr_ver.count(p))
              convergence = false;

        // Add phis at convergence points, then update vers. Do this until
        // no blocks have multiple versions of the variable.
        if (convergence) {
          cout << "Convergence point at BB " << b->id << endl;

          if_val *phi = new if_val();
          phi->op = VAL_PHI;
          phi->t = (*ver[b].begin())->t;
          phi->id = id++;
          phi->bb = b;
          for (auto &v : ver[b])
            phi->args.push_back(v);
          b->vals.insert(b->vals.begin(), phi);
          id_to_val[x.first].insert(phi);
          val_to_id[phi] = x.first;
          // No args: should not have to 
          
          wr_ver[b] = phi;
          phi_added = true;
        }
      }

      // Now that we know which version to use, assign args and set
      // live_out accordingly.
      for (auto &b : f->bbs) {
        if (ver[b].size() != 1) continue;

        if_val *current_ver = *(ver[b].begin());
        for (auto &v : b->vals) {
          if (predicate.count(v) && predicate[v] == x.first)
            v->pred = current_ver;
          for (unsigned i = 0; i < arg_ids[v].size(); ++i)
            if (arg_ids[v][i] == x.first)
              v->args[i] = current_ver;
          if (val_to_id[v] == x.first) current_ver = v;
        }
        if (br_id.count(b) && br_id[b] == x.first) b->branch_pred = current_ver;

        if (live_out[b].count(x.first)) {
          bool found = false;
          for (auto &v : b->live_out)
            if (v == current_ver) found = true;
          if (!found)
            b->live_out.push_back(current_ver);
        }
      }
    } while (phi_added);
  }

  // Find live_in for all blocks, as the union of all predecessors' live_out
  for (auto &b : f->bbs) {
    set<if_val *> live_in;
    for (auto &s : b->pred)
      for (auto &v : s->live_out)
        live_in.insert(v);
    for (auto &v : live_in)
      b->live_in.push_back(v);
  }

  // Flag back edges for 2-entry pipeline buffers.
  break_cycles(*f);
  
  // Prevent repeat assembly.
  f = NULL;
}
