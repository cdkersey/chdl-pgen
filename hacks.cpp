#include <iostream>

#include <vector>

namespace bscotch {
  // Types are represented by vectors of integers.
  enum type_part {
    TYPE_BIT = -100,
    TYPE_S = -101,             // Followed by #bits
    TYPE_U = -102,             // Followed by #bits
    TYPE_ARRAY = -1000,        // Followed by dimension, then array type.
    TYPE_STATIC_ARRAY = -1001, // Followed by dimension, then array type.
    TYPE_STRUCT_BEGIN = -1002, // Followed by #elements.
    TYPE_FIELD_DELIM = -1003   // Next element of struct.
  };

  // TODO: add operations on types.
  struct type {
   vector<int> type_vec;
  }
  
  enum if_op {
    VAL_CONST,
    VAL_PHI, VAL_SELECT,
    VAL_ARG,
    VAL_LD_STATIC, VAL_ST_STATIC,
    VAL_LD_IDX, VAL_ST_IDX,
    VAL_LD_IDX_STATIC, VAL_ST_IDX_STATIC,
    VAL_NEG, VAL_NOT,
    VAL_ADD, VAL_SUB, VAL_MUL, VAL_DIV, VAL_AND, VAL_OR, VAL_XOR,
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
    type type;
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

using namespace bscotch;
using namespace std;

void print(std::ostream &out, type &t) {
}

void print(std::ostream &out, if_val &v) {
  cout << if_op_str[v.op] << endl;
}

void print(std::ostream &out, if_bb &b) {
  for (auto &v : b.vals)
    print(out, v);
}

void print(std::ostream &out, if_func &f) {
  for (auto &b : f.bbs)
    print(out, b);
}

void test_func(if_func &f) {
  // Test function:
}

int main() {
  if_func f;

  test_func(f);

  print(cout, f);

  return 0;
}
