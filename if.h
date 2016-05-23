#ifndef BSCOTCH_IF_H
#define BSCOTCH_IF_H

#include <iostream>

#include <vector>
#include <string>
#include <sstream>

#include "type.h"

namespace bscotch {
  enum if_op {
    VAL_CONST,
    VAL_PHI, VAL_SELECT,
    VAL_ARG,
    VAL_LD_STATIC, VAL_ST_STATIC,
    VAL_LD_IDX, VAL_ST_IDX,
    VAL_LD_IDX_STATIC, VAL_ST_IDX_STATIC,
    VAL_NEG, VAL_NOT,
    VAL_ADD, VAL_SUB, VAL_MUL, VAL_DIV, VAL_AND, VAL_OR, VAL_XOR,
    VAL_CONCATENATE,
    VAL_CALL_STATIC, VAL_CALL,
    VAL_BRANCH
  };

  const char *if_op_str[] = {
    "const", "phi", "select", "arg",
    "ld_static", "st_static", "ld_idx", "st_idx_static", "ld_idx_static",
    "neg", "not",
    "add", "sub", "mul", "div", "and", "or", "xor"
    "call_static", "call",
    "branch"
  };
  
  struct if_val {
    std::vector<if_val *> args;

    // If VAL_CONST, this contains the value
    std::vector<bool> const_val;
    
    if_op op;
    type t;
  };

  struct if_bb {
    std::vector<if_val> vals;

    // Set by liveness analysis.
    std::vector<if_val*> live;

    // Successors; may be NULL.
    if_bb *suc_0, *suc_1;
  };

  struct if_func {
    std::vector<type> args;
    std::vector<if_bb> bbs;
  };
};

void print(std::ostream &out, bscotch::if_val &v) {
  using namespace std;
  using namespace bscotch;

  cout << if_op_str[v.op] << " (";
  print(out, v.t);
  cout << ')' << endl;
}

void print(std::ostream &out, bscotch::if_bb &b) {
  using namespace std;
  using namespace bscotch;

  for (auto &v : b.vals)
    print(out, v);
}

void print(std::ostream &out, bscotch::if_func &f) {
  using namespace std;
  using namespace bscotch;

  for (auto &b : f.bbs)
    print(out, b);
}

#endif
