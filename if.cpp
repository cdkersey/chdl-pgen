#include <iostream>

#include <vector>
#include <map>
#include <string>
#include <sstream>

#include "if.h"
#include "type.h"

const char *bscotch::if_op_str[] = {
  "const", "phi", "select", "arg",
  "ld_global", "st_global",
  "ld_static", "st_static",
  "ld_idx", "st_idx", "ld_idx_static", "st_idx_static",
  "neg", "not",
  "add", "sub", "mul", "div", "and", "or", "xor",
  "cat", "call_static", "call"
};

// Print vector of bools as hex. TODO: move to separate utility library?
std::string bscotch::to_hex(std::vector<bool> &v) {
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

void bscotch::print(std::ostream &out, std::vector<bool> &v) {
  out << to_hex(v);
}

void bscotch::print(std::ostream &out, bscotch::if_val &v) {
  using namespace std;
  using namespace bscotch;

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
      out << " <" << a->id << '>';
    }
  }

  cout << " (";
  print(out, v.t);
  out << ')' << endl;
}

void bscotch::print(std::ostream &out, bscotch::if_bb &b) {
  using namespace std;
  using namespace bscotch;

  for (auto &v : b.vals)
    print(out, v);

  out << "    br ";
  if (b.branch_pred) out << '<' << b.branch_pred->id << ">, ";
  for (unsigned i = 0; i < b.suc.size(); ++i) {
    out << "bb " << b.suc[i]->id;
    if (i != b.suc.size() - 1) out << ", ";
  }
  out << endl;
}

void bscotch::print(std::ostream &out, bscotch::if_staticvar &v) {
    print(out, v.t);
    out << " = 0x";
    print(out, v.initial_val);
}

void bscotch::print(std::ostream &out, bscotch::if_func &f) {
  using namespace std;
  using namespace bscotch;

  for (auto &v : f.static_vars) {
    out << "  " << v.first << " : ";
    print(out, v.second);
    out << endl;
  }
  
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    out << "  bb " << i << ':' << endl;
    print(out, f.bbs[i]);
  }
}

void bscotch::print(std::ostream &out, bscotch::if_prog &p) {
  using namespace std;
  using namespace bscotch;

  for (auto &v : p.global_vars) {
    out << v.first << " : ";
    print(out, v.second);
    out << endl;
  }
  
  for (auto &f : p.functions) {
    out << f.first << ": " << endl;
    print(out, f.second);
  }
}

unsigned long bscotch::const_val(const bscotch::if_val &v) {
  unsigned long r = 0;

  for (unsigned i = 0; i < v.const_val.size(); ++i)
    if (v.const_val[i])
      r += (1<<i);
  
  return r;
}
