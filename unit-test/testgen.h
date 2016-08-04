#ifndef BSCOTCH_TESTGEN_H
#define BSCOTCH_TESTGEN_H

#include <vector>
#include <string>
#include <functional>

#include "../if.h"

namespace bscotch {
  class test_registrator {
  public:
    test_registrator(const char *name, void (*f)(if_prog *p)) :
      name(name), f(f)
    { tests.push_back(this); }

    void run(if_prog *p) { f(p); }

    static void run_tests();
    
  private:
    std::string name;
    std::function<void(if_prog *p)> f;

    static std::vector<test_registrator *> tests;
  };
};

#define REGISTER_TEST(name, func) \
  bscotch::test_registrator test_reg_##name(#name, (func));

#endif
