#include <iostream>

#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"

using namespace bscotch;
using namespace std;

void test_func(if_func &f) {
  // Test function:
}

int main() {
  if_func f;

  test_func(f);

  print(cout, f);

  return 0;
}
