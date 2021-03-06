#ifndef CHDL_PGEN_IF_H
#define CHDL_PGEN_IF_H

#include <iostream>

#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <list>

#include "type.h"

namespace pgen {
  enum if_op {
    VAL_CONST,
    VAL_PHI, VAL_SELECT,
    VAL_ARG,
    VAL_LD_STATIC, VAL_ST_STATIC,
    VAL_LD_IDX, VAL_ST_IDX,
    VAL_LD_IDX_STATIC, VAL_ST_IDX_STATIC,
    VAL_BCAST_VALID_STATIC,
    VAL_NEG, VAL_NOT,
    VAL_ADD, VAL_SUB, VAL_MUL, VAL_DIV, VAL_AND, VAL_OR, VAL_XOR,
    VAL_SHR, VAL_SHL,
    VAL_EQ, VAL_LT,
    VAL_AND_REDUCE, VAL_OR_REDUCE,
    VAL_CONCATENATE, VAL_BUILD,
    VAL_CALL_STATIC, VAL_CALL, VAL_SPAWN, VAL_RET
  };

  extern const char *if_op_str[];

  struct if_staticvar {
    std::string name;
    type t;
    std::vector<bool> initial_val;

    // The code generators need to know how many loads and stores have each
    // static variable as a destination.
    unsigned store_count, load_count;
    
    // Broadcast variables are non-persistent intra-cycle communication between
    // pipeline stages.
    bool broadcast;
  };

  struct if_bb;
  
  struct if_val {
    std::vector<if_val *> args;
    if_staticvar *static_arg;

    // Function argument for calls.
    std::string func_arg;
    
    // Loads and stores to static_arg are counted by the code generator. When
    // this occurs, each unique access is given an ID.
    // For VAL_ARG, this contains the argument index.
    unsigned static_access_id;
    
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
    std::vector<if_val*> vals;

    // Index in function's basic block vector.
    int id;
    
    // Set by liveness analysis.
    std::vector<if_val*> live_in, live_out;

    // If this is true, a more expensive 2-entry pipeline register is used.
    // This allows a cycle of slack and places a break in the ready signal
    // generation chain.
    bool cycle_breaker;

    // Stall predicate; a currently live val
    if_val *stall;
    
    // Branch predicate; a currently live val (if needed)
    if_val *branch_pred;

    // Predecessor blocks.
    std::vector<if_bb *> pred;

    // Successor blocks, organized into branch target groups. A branch predicate
    // value can lead to 0(cancel thread) or more successor blocks. Having more
    // than one successor in a single target group spawns new threads.
    std::vector<std::list<if_bb *> > suc_l;
  };

  struct if_func {
    std::map<std::string, if_staticvar> static_vars;
    type rtype;
    std::vector<type> args;
    std::vector<if_bb*> bbs;
  };

  struct if_prog {
    std::map<std::string, if_func> functions;
  };

  unsigned long const_val(const pgen::if_val &v);
  
  std::string to_hex(std::vector<bool> &v);
  void print(std::ostream &out, std::vector<bool> &v);
  void print(std::ostream &out, pgen::if_val &v);
  void print(std::ostream &out, pgen::if_bb &b);
  void print(std::ostream &out, pgen::if_staticvar &v);
  void print(std::ostream &out, pgen::if_func &f);
  void print(std::ostream &out, pgen::if_prog &p);
}

#endif
