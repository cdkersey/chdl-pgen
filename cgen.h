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
  void gen_prog(std::ostream &out, if_prog &p);
}

#endif
