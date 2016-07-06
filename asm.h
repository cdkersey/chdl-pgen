#ifndef BSCOTCH_ASM_H
#define BSCOTCH_ASM_H

#include <iostream>

#include <vector>
#include <map>
#include <string>
#include <sstream>

#include "type.h"
#include "if.h"

namespace bscotch {
  struct asm_prog {
    typedef unsigned val_id_t;
    
    asm_prog(if_prog &p): p(p), f(0), b(0), v(0) {}
    
    void function(std::string name);
    void label(std::string name);
    void static_var(const type &t, std::string name);
    void bcast_var(const type &t, std::string name);
    template <typename T>
      void static_var(const type &t, std::string name, const T &initial_val);

    asm_prog &val(const type &t, val_id_t id, if_op op);
    asm_prog &val(val_id_t id, if_op op);
    asm_prog &arg(val_id_t id);
    asm_prog &const_arg(long const_arg);
    asm_prog &static_arg(std::string static_name);

    asm_prog &br(val_id_t sel);
    asm_prog &br();
    asm_prog &target(std::string label);

    std::map<std::string, if_bb*> labels;
    
    if_prog &p;
    if_func *f;
    if_bb *b;
    if_val *v;
  };
}

template <typename T>
  void bscotch::asm_prog::static_var(
    const type &t, std::string name, const T &initial_val
  )
{
  static_var(t, name);
  to_vec_bool(f->static_vars[name].initial_val, initial_val);
}

#endif
