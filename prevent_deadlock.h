#ifndef BSCOTCH_PREVENT_DEADLOCK
#define BSCOTCH_PREVENT_DEADLOCK

#include "if.h"

namespace bscotch {
  // Reorder predecessor blocks to prevent deadlock.
  void prevent_deadlock(if_func &f);
};

#endif
