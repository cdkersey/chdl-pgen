#ifndef CHDL_PGEN_ASM_H
#define CHDL_PGEN_ASM_H

#include <iostream>

#include <vector>
#include <list>
#include <map>
#include <set>
#include <string>
#include <sstream>

#include "type.h"
#include "if.h"

namespace pgen {
  struct asm_prog {
    typedef unsigned val_id_t;
    
    asm_prog(if_prog &p): p(p), f(0), b(0), v(0) {}

    void assemble_func(); // Resolve all argument IDs, etc. in current function.
    void bb_resolveptrs(); // Resolve suc/pred ptrs in func's basic blocks.
    
    void function(std::string name); // New function; assembler current one.
    void label(std::string name);
    void static_var(const type &t, std::string name);
    void bcast_var(const type &t, std::string name);
    template <typename T>
      void static_var(const type &t, std::string name, const T &initial_val);

    asm_prog &val(const type &t, val_id_t id, if_op op);
    asm_prog &val(val_id_t id, if_op op);
    asm_prog &arg(val_id_t id);
    asm_prog &pred(val_id_t id);
    asm_prog &const_arg(long const_arg);
    asm_prog &static_arg(std::string static_name);
    asm_prog &func_arg(std::string func_name);

    asm_prog &br(val_id_t sel);
    asm_prog &br();
    asm_prog &stall(val_id_t in);
    asm_prog &target(std::string label);
    asm_prog &empty_tgroup();
    asm_prog &tgroup_add(std::string label);    

    std::map<std::string, if_bb*> labels;
    std::map<val_id_t, std::set<if_val*> > id_to_val;
    std::map<if_val*, std::vector<val_id_t> > arg_ids;
    std::map<if_val*, val_id_t> predicate;
    std::map<if_bb*, val_id_t> br_id;
    std::map<if_bb*, val_id_t> stall_id;
    std::map<if_bb*, std::vector<std::list<std::string> > > br_targets;

    std::string func_name;
    
    if_prog &p;
    if_func *f;
    if_bb *b;
    if_val *v;
  };

  template <typename T>
    void to_vec_bool(std::vector<bool> &v, unsigned size, T val);
}

template <typename T>
  void pgen::to_vec_bool(std::vector<bool> &v, unsigned size, T val)
{
  for (unsigned i = 0; i < size; ++i)
    v.push_back((val & (1ul<<i)) ? true : false);
}

template <typename T>
  void pgen::asm_prog::static_var(
    const type &t, std::string name, const T &initial_val
  )
{
  static_var(t, name);
  to_vec_bool(f->static_vars[name].initial_val, t.size(), initial_val);
}

#endif
