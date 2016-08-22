#include <map>

#include "if.h"
#include "remove_copies.h"

using namespace std;
using namespace pgen;

void pgen::remove_copies(if_func &f) {
  bool changed;
  
  map<if_val*, if_val*> m;
  
  do {
    changed = false;
    
    for (auto &b : f.bbs) {
      set<unsigned> to_remove;

      if (m.count(b->stall)) {
        changed = true;
        b->stall = m[b->stall];
      }

      if (m.count(b->branch_pred)) {
        changed = true;
        b->branch_pred = m[b->branch_pred];
      }

      for (auto &v : b->live_in) {
        if (m.count(v)) {
          changed = true;
          v = m[v];
        }
      }

      for (auto &v : b->live_out) {
        if (m.count(v)) {
          changed = true;
          v = m[v];
        }
      }
      
      for (auto &v : b->vals) {
        if (m.count(v->pred)) {
          changed = true;
          v->pred = m[v->pred];
        }

        for (unsigned i = 0; i < v->args.size(); ++i) {
          if (m.count(v->args[i])) {
            changed = true;
            v->args[i] = m[v->args[i]];
          }
        }
      }
      
      for (unsigned i = 0; i < b->vals.size(); ++i) {
        if (b->vals[i]->op == VAL_CONCATENATE && b->vals[i]->args.size() == 1) {
          changed = true;
          to_remove.insert(i);
          m[b->vals[i]] = b->vals[i]->args[0];
        }
      }

      vector<if_val*> new_v;
      for (unsigned i = 0; i < b->vals.size(); ++i)
        if (!to_remove.count(i))
          new_v.push_back(b->vals[i]);
      b->vals = new_v;
    }
  } while (changed);

}
