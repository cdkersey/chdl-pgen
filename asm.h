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
    
    asm_prog(if_prog &p): p(p) {}
    
    void function(std::string name);
    void label(std::string name);
    void static_var(std::string name, bool bcast = false);

    asm_prog &val(type &t, val_id_t id, if_op op);
    asm_prog &val(val_id_t id, if_op op);
    asm_prog &arg(val_id_t id);
    asm_prog &arg(unsigned long const_arg);
    asm_prog &arg(std::string static_name);

    asm_prog &br(val_id_t sel);
    asm_prog &br();
    asm_prog &target(std::string label);

    if_prog &p;
  };
  
}

#endif
