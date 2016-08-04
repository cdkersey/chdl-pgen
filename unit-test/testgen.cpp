#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"
#include "../cgen.h"
#include "../cgen-cpp.h"
#include "../break_cycles.h"
#include "../asm-macro.h"

#include "testgen.h"

using namespace bscotch;

std::vector<bscotch::test_registrator*> test_registrator::tests;

void bscotch::test_registrator::run_tests() {
  for (auto &t : tests) {
    if_prog p;
    t->run(&p);

    std::ofstream asm_out(t->name + ".s");
    print(asm_out, p);

    std::ofstream chdl_out(t->name + ".chdl.cpp");
    gen_prog(chdl_out, p);

    std::ofstream cpp_out(t->name + ".sim.cpp");
    gen_prog_cpp(cpp_out, p);
  }
}

int main(int argc, char **argv) {
  bscotch::test_registrator::run_tests();

  return 0;
}
