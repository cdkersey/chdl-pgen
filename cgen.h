#ifndef BSCOTCH_CGEN_H
#define BSCOTCH_CGEN_H

#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#include "type.h"
#include "if.h"

namespace bscotch {
  void gen_val(std::ostream &out, std::string fname, int bbidx, int idx, if_bb &b, if_val &v);
  void gen_bb_decls(std::ostream &out, std::string fname, int idx, if_bb &b, bool entry);
  void gen_bb(std::ostream &out, std::string fname, int idx, if_bb &b, bool entry);
  void gen_func_decls(std::ostream &out, std::string name, if_func &f);
  void gen_func(std::ostream &out, std::string name, if_func &f);
  void gen_prog(std::ostream &out, if_prog &p);
}

#endif
