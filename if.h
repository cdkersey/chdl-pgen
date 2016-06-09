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

  extern const char *if_op_str[];

  struct if_staticvar {
    std::string name;
    type t;
    std::vector<bool> initial_val;
  };

  struct if_bb;
  
  struct if_val {
    std::vector<if_val *> args;
    if_staticvar *static_arg;
    
    // For VAL_CONST, this contains the value
    std::vector<bool> const_val;

    // If non-NULL, this instruction's execution is predicated on this value.
    // Currently used only for static variable stores.
    if_val *pred;
    
    if_op op;
    type t;
    int id;
    if_bb *bb;
  };

  struct if_bb {
    std::vector<if_val> vals;

    // Index in function's basic block vector.
    int id;
    
    // Set by liveness analysis.
    std::vector<if_val*> live_in, live_out;

    // Branch predicate, one of this block's vals (if needed)
    if_val *branch_pred;
    
    // Successor and predecessor blocks.
    std::vector<if_bb *> suc, pred;
  };

  struct if_func {
    std::map<std::string, if_staticvar> static_vars;
    type rtype;
    std::vector<type> args;
    std::vector<if_bb> bbs;
  };

  struct if_prog {
    std::map<std::string, if_staticvar> global_vars;
    std::map<std::string, if_func> functions;
  };

  std::string to_hex(std::vector<bool> &v);
  void print(std::ostream &out, std::vector<bool> &v);
  void print(std::ostream &out, bscotch::if_val &v);
  void print(std::ostream &out, bscotch::if_bb &b);
  void print(std::ostream &out, bscotch::if_staticvar &v);
  void print(std::ostream &out, bscotch::if_func &f);
  void print(std::ostream &out, bscotch::if_prog &p);
}

#endif
