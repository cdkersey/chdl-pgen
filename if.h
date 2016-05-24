#ifndef BSCOTCH_IF_H
#define BSCOTCH_IF_H

#include <iostream>

#include <vector>
#include <map>
#include <string>
#include <sstream>

#include "type.h"

namespace bscotch {
  enum if_op {
    VAL_CONST,
    VAL_PHI, VAL_SELECT,
    VAL_ARG,
    VAL_LD_GLOBAL, VAL_ST_GLOBAL,
    VAL_LD_STATIC, VAL_ST_STATIC,
    VAL_LD_IDX, VAL_ST_IDX,
    VAL_LD_IDX_STATIC, VAL_ST_IDX_STATIC,
    VAL_NEG, VAL_NOT,
    VAL_ADD, VAL_SUB, VAL_MUL, VAL_DIV, VAL_AND, VAL_OR, VAL_XOR,
    VAL_CONCATENATE,
    VAL_CALL_STATIC, VAL_CALL,
  };

  const char *if_op_str[] = {
    "const", "phi", "select", "arg",
    "ld_global", "st_global",
    "ld_static", "st_static",
    "ld_idx", "st_idx_static", "ld_idx_static",
    "neg", "not",
    "add", "sub", "mul", "div", "and", "or", "xor"
    "call_static", "call",
  };

  struct if_staticvar {
    std::string name;
    type t;
    std::vector<bool> initial_val;
  };
  
  struct if_val {
    std::vector<if_val *> args;
    if_staticvar *static_arg;
    
    // If VAL_CONST, this contains the value
    std::vector<bool> const_val;
    
    if_op op;
    type t;
  };

  struct if_bb {
    std::vector<if_val> vals;

    // Set by liveness analysis.
    std::vector<if_val*> live;

    // Branch predicate, one of this block's vals (if needed)
    if_val *branch_pred;
    
    // Successors; may be NULL.
    std::vector<if_bb *> suc;
  };

  struct if_func {
    std::map<std::string, if_staticvar> static_vars;
    std::vector<type> args;
    std::vector<if_bb> bbs;
  };

  struct if_prog {
    std::map<std::string, if_staticvar> global_vars;
    std::map<std::string, if_func> functions;
  };
};

void print(std::ostream &out, bscotch::if_val &v) {
  using namespace std;
  using namespace bscotch;

  out << "    ";
  out << if_op_str[v.op];

  /* TODO: Print args */
  
  cout << " (";
  print(out, v.t);
  out << ')' << endl;
}

void print(std::ostream &out, bscotch::if_bb &b) {
  using namespace std;
  using namespace bscotch;

  for (auto &v : b.vals)
    print(out, v);
}

// Print vector of bools as hex. TODO: move to separate utility library?
void print(std::ostream &out, std::vector<bool> &v) {
  if (v.size() == 0) out << '0';

  for (int i = (v.size() + 3)/4*4 - 4; i >= 0; i -= 4) {
    unsigned x = 0;
    if (v.size() > i + 0 && v[i + 0]) x += 1;
    if (v.size() > i + 1 && v[i + 1]) x += 2;
    if (v.size() > i + 2 && v[i + 2]) x += 4;
    if (v.size() > i + 3 && v[i + 3]) x += 8;
    out << "0123456789abcdef"[x];
  }
}

void print(std::ostream &out, bscotch::if_staticvar &v) {
    print(out, v.t);
    out << " = ";
    print(out, v.initial_val);
}

void print(std::ostream &out, bscotch::if_func &f) {
  using namespace std;
  using namespace bscotch;

  for (auto &v : f.static_vars) {
    out << "  " << v.first << " : ";
    print(out, v.second);
    out << endl;
  }
  
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    out << "  bb " << i << ':' << endl;
    print(out, f.bbs[i]);
  }
}

void print(std::ostream &out, bscotch::if_prog &p) {
  using namespace std;
  using namespace bscotch;

  for (auto &v : p.global_vars) {
    out << v.first << " : ";
    print(out, v.second);
    out << endl;
  }
  
  for (auto &f : p.functions) {
    out << f.first << ": " << endl;
    print(out, f.second);
  }

}

#endif
