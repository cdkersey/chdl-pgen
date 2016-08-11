#include <iostream>
#include <fstream>

#include <vector>
#include <string>
#include <sstream>

#include "../type.h"
#include "../if.h"
#include "../cgen.h"
#include "../cgen-cpp.h"
#include "../break_cycles.h"
#include "../asm.h"

#include "testgen.h"

using namespace pgen;
using namespace std;

void asm1(if_prog *pp) {
  // Initialize the assembler.
  if_prog &p(*pp);
  asm_prog a(p);
  
  // The assembly program.
  a.function("bmain");

  a.label("entry");
  a.val(u(32), 0,     VAL_CONST).   const_arg(0);
  a.val(u(32), 1,     VAL_CONST).   const_arg(0);

  a.label("loop");
  a.val(u(32), 2,     VAL_CONST).const_arg(1103515245);
  a.val(u(32), 3,     VAL_CONST).const_arg(12345);
  a.val(u(32), 4,       VAL_MUL).arg(1).arg(2);
  a.val(u(32), 1,       VAL_ADD).arg(3).arg(4);
  a.val(u(32), 5,     VAL_CONST).const_arg(100);
  a.val(u(32), 6,       VAL_XOR).arg(0).arg(5);
  a.val(bit(), 7, VAL_OR_REDUCE).arg(6);
  a.val(bit(), 7,       VAL_NOT).arg(7);
  a.val(u(32), 8,     VAL_CONST).const_arg(1);
  a.val(u(32), 0,       VAL_ADD).arg(0).arg(8);
  a.val(void_type(), 9, VAL_SPAWN).func_arg("func").arg(1);
  a.br(7).target("loop").target("exit");

  a.label("exit");
  a.val(void_type(), 10, VAL_RET);

  a.function("func");

  a.bcast_var(u(3), "d1");
  a.bcast_var(u(3), "d2");
  a.bcast_var(u(3), "d3");

  a.label("stage_0");
  a.val(u(32), 0, VAL_ARG);
  a.val(u(5), 1, VAL_CONST).const_arg(0);
  a.val(u(5), 2, VAL_CONST).const_arg(3);
  a.val(u(5), 3, VAL_CONST).const_arg(6);
  a.val(u(3), 4, VAL_LD_IDX).arg(0).arg(1).arg(2);
  a.val(u(3), 5, VAL_LD_IDX).arg(0).arg(2).arg(2);
  a.val(u(3), 6, VAL_LD_IDX).arg(0).arg(3).arg(2);
  a.val(u(3), 7, VAL_LD_STATIC).static_arg("d1");
  a.val(u(3), 8, VAL_LD_STATIC).static_arg("d2");
  a.val(u(3), 9, VAL_LD_STATIC).static_arg("d3");
  a.val(u(3), 10, VAL_XOR).arg(5).arg(7);
  a.val(u(3), 11, VAL_XOR).arg(5).arg(8);
  a.val(u(3), 12, VAL_XOR).arg(5).arg(9);
  a.val(u(3), 13, VAL_XOR).arg(6).arg(7);
  a.val(u(3), 14, VAL_XOR).arg(6).arg(8);
  a.val(u(3), 15, VAL_XOR).arg(6).arg(9);
  a.val(bit(), 16, VAL_OR_REDUCE).arg(10);
  a.val(bit(), 17, VAL_OR_REDUCE).arg(11);
  a.val(bit(), 18, VAL_OR_REDUCE).arg(12);
  a.val(bit(), 19, VAL_OR_REDUCE).arg(13);
  a.val(bit(), 20, VAL_OR_REDUCE).arg(14);
  a.val(bit(), 21, VAL_OR_REDUCE).arg(15);
  a.val(u(6), 22, VAL_CONCATENATE);
  for (unsigned i = 16; i <= 21; ++i) a.arg(i);
  a.val(bit(), 23, VAL_BCAST_VALID_STATIC).static_arg("d1");
  a.val(bit(), 24, VAL_BCAST_VALID_STATIC).static_arg("d2");
  a.val(bit(), 25, VAL_BCAST_VALID_STATIC).static_arg("d3");
  a.val(u(6), 26, VAL_CONCATENATE).
    arg(23).arg(24).arg(25).arg(23).arg(24).arg(25);
  a.val(u(6), 27, VAL_NOT).arg(22);
  a.val(u(6), 28, VAL_AND).arg(26).arg(27);
  a.val(bit(), 29, VAL_OR_REDUCE).arg(28);
  a.stall(29);
  
  a.label("stage_1");
  a.val(void_type(), 31, VAL_ST_STATIC).static_arg("d1").arg(4);

  a.label("stage_2");
  a.val(void_type(), 32, VAL_ST_STATIC).static_arg("d2").arg(4);

  a.label("stage_3");
  a.val(void_type(), 33, VAL_ST_STATIC).static_arg("d3").arg(4);
  a.val(void_type(), 35, VAL_RET);
  
  a.assemble_func();
}

REGISTER_TEST(asm1, asm1);
