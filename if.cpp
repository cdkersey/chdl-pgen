#include <iostream>

#include <vector>
#include <map>
#include <string>
#include <sstream>

#include "if.h"
#include "type.h"

const char *pgen::if_op_str[] = {
  "const", "phi", "select", "arg",
  "ld_static", "st_static",
  "ld_idx", "st_idx",
  "ld_idx_static", "st_idx_static",
  "bcast_valid_static",
  "neg", "not",
  "add", "sub", "mul", "div", "and", "or", "xor",
  "shr", "shl",
  "eq", "lt",
  "and_reduce", "or_reduce",
  "cat", "build",
  "call_static", "call", "spawn", "ret"
};

// Print vector of bools as hex. TODO: move to separate utility library?
std::string pgen::to_hex(std::vector<bool> &v) {
  std::ostringstream out;

  if (v.size() == 0) out << '0';

  for (int i = (v.size() + 3)/4*4 - 4; i >= 0; i -= 4) {
    unsigned x = 0;
    if (v.size() > i + 0 && v[i + 0]) x += 1;
    if (v.size() > i + 1 && v[i + 1]) x += 2;
    if (v.size() > i + 2 && v[i + 2]) x += 4;
    if (v.size() > i + 3 && v[i + 3]) x += 8;
    out << "0123456789abcdef"[x];
  }

  return out.str();
}

void pgen::print(std::ostream &out, std::vector<bool> &v) {
  out << to_hex(v);
}

void pgen::print(std::ostream &out, pgen::if_val &v) {
  using namespace std;
  using namespace pgen;

  out << "    ";
  if (v.pred) out << '<' << v.pred->id << "> ? ";
  out << '<' << v.id << "> = " << if_op_str[v.op];

  if (v.op == VAL_CONST) {
    out << " 0x";
    print(out, v.const_val);
  } else {
    if (v.static_arg) out << " @" << v.static_arg->name;
    if (v.func_arg != "") out << ' ' << v.func_arg;
    bool comma = (v.static_arg || v.func_arg != "" ? 1 : 0);
    for (auto &a : v.args) {
      if (comma) out << ',';
      else comma = true;
      if (a) out << " <" << a->id << '>';
      else out << " <NULL>";
    }
  }

  out << " (";
  print(out, v.t);
  out << ')' << endl;
}

void pgen::print(std::ostream &out, pgen::if_bb &b) {
  using namespace std;
  using namespace pgen;

  out << "    Live in:";
  for (auto &v : b.live_in) out << ' ' << v->id;
  out << endl << "    Live out:";
  for (auto &v : b.live_out) out << ' ' << v->id;
  out << endl;
  
  for (auto &v : b.vals)
    print(out, *v);

  if (b.stall) out << "    stall <" << b.stall->id << '>' << endl;
  
  out << "    br ";
  if (b.branch_pred) out << '<' << b.branch_pred->id << ">, ";
  for (unsigned i = 0; i < b.suc_l.size(); ++i) {
    if (b.suc_l[i].size() != 1) out << '(';
    for (auto it = b.suc_l[i].begin(); it != b.suc_l[i].end(); ++it) {
      if (it != b.suc_l[i].begin()) out << ' ';
      out << "bb" << (*it)->id;      
    }
    if (b.suc_l[i].size() != 1) out << ')';
    if (i != b.suc_l.size() - 1) out << ", ";
  }
  out << endl;
}

void pgen::print(std::ostream &out, pgen::if_staticvar &v) {
    print(out, v.t);
    out << " = 0x";
    print(out, v.initial_val);
}

void pgen::print(std::ostream &out, pgen::if_func &f) {
  using namespace std;
  using namespace pgen;

  for (auto &v : f.static_vars) {
    out << "  " << v.first << " : ";
    print(out, v.second);
    out << endl;
  }
  
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    out << "  bb " << i << ':' << endl;
    print(out, *f.bbs[i]);
  }
}

void pgen::print(std::ostream &out, pgen::if_prog &p) {
  using namespace std;
  using namespace pgen;

  for (auto &f : p.functions) {
    out << f.first << ": " << endl;
    print(out, f.second);
  }
}

unsigned long pgen::const_val(const pgen::if_val &v) {
  unsigned long r = 0;

  for (unsigned i = 0; i < v.const_val.size(); ++i)
    if (v.const_val[i])
      r += (1<<i);
  
  return r;
}
