#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#include "type.h"
#include "if.h"
#include "cgen-cpp.h"

using namespace bscotch;

static void gen_func(std::ostream &out, std::string name, if_func &f) {
  using namespace std;

  out << "// " << name << " definition." << endl;
}

static void gen_func_decls(std::ostream &out, std::string name, if_func &f) {
  using namespace std;

  out << "// " << name << " declarations." << endl;

  // Call and return types.
  out << "struct " << name << "_call_t {" << endl
      << "  bool valid;" << endl
      << "  void *live;" << endl;
  for (unsigned i = 0; i < f.args.size(); ++i) {
    ostringstream arg_name;
    arg_name << "arg" << i;
    out << "  " << type_cpp(f.args[i], arg_name.str()) << ';' << endl;
  }
    
  out << "};" << endl << endl
      << "struct " << name << "_ret_t {" << endl
      << "  bool valid;" << endl
      << "  void *live;" << endl;
  if (!is_void_type(f.rtype))
      out << "  " << type_cpp(f.rtype, "rval") << ';' << endl;
  out << "};" << endl << endl;
  
  // Input and output types for every basic block.
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    out << "struct " << name << "_bb" << i << "_in_t {" << endl
        << "  bool valid;" << endl
        << "  void *live;" << endl;
    for (unsigned j = 0; j < f.bbs[i]->live_in.size(); ++j) {
      if_val *v = f.bbs[i]->live_in[j];
      ostringstream var_name;
      var_name << "val" << v->id;
      out << "  " << type_cpp(v->t, var_name.str()) << ';' << endl;
    }
    out << "};" << endl << endl;

    out << "struct " << name << "_bb" << i << "_out_t {" << endl
        << "  bool valid;" << endl
        << "  void *live;" << endl;
    for (unsigned j = 0; j < f.bbs[i]->live_out.size(); ++j) {
      if_val *v = f.bbs[i]->live_out[j];
      ostringstream var_name;
      var_name << "val" << v->id;
      out << "  " << type_cpp(v->t, var_name.str()) << ';' << endl;
    }
    out << "};" << endl << endl;
  }
  
  out << "struct " << name << "_state_t {" << endl;
  for (unsigned i = 0; i < f.bbs.size(); ++i) {
    out << "  " << name << "_bb" << i << "_in_t bb" << i << "_in";
    if (f.bbs[i]->cycle_breaker)
      out << ", bb" << i << "_in_tmp";
    out << ';' << endl;
    out << "  " << name << "_bb" << i << "_out_t bb" << i << "_out;" << endl;
  }
  out << "};" << endl << endl;
}

void bscotch::gen_prog_cpp(std::ostream &out, if_prog &p) {
  for (auto &f : p.functions)
    gen_func_decls(out, f.first, f.second);

  for (auto &f : p.functions)
    gen_func(out, f.first, f.second);
}
