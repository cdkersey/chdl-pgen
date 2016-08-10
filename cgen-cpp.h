#ifndef CHDL_PGEN_CGEN_CPP_H
#define CHDL_PGEN_CGEN_CPP_H

#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#include "type.h"
#include "if.h"

namespace pgen {
  void gen_prog_cpp(std::ostream &out, if_prog &p);
}

#endif
