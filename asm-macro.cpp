#include "asm-macro.h"

#include <cstdlib>

using namespace bscotch;
using namespace std;

static asm_prog *asm_prog_ptr;
vector<bscotch::varimpl *> vars;

struct bscotch::varimpl {
  varimpl(): v(NULL), id(next_id++), generic(true) {
    vars.push_back(this);
  }
  
  varimpl(const type &t): t(t), v(NULL), id(next_id++), generic(false) {
    vars.push_back(this);
  }

  bscotch::asm_prog::val_id_t id;
  if_val *v; // Most recently assigned value. 
  type t; // Type.
  bool generic; // Type not yet assigned.

  static bscotch::asm_prog::val_id_t next_id;
};

bscotch::asm_prog::val_id_t bscotch::varimpl::next_id;

// Initialize the macro API. This is a thread-unsafe, stateful API designed as
// a stop-gap solution between the assembler and a language front-end/parser.
void bscotch::init_macro_env(asm_prog &a) {
  asm_prog_ptr = &a;
  bscotch::varimpl::next_id = 0;
  for (auto &p : vars) delete p;
  vars.clear();
}

static bool is_void_type(const type &t) { return t.type_vec.size() == 0; }

static void infer_type(if_val *v, type *t) {
  bool is_store(v->op == VAL_ST_IDX || v->op == VAL_ST_IDX_STATIC),
       is_static(v->op == VAL_LD_IDX_STATIC || v->op == VAL_ST_IDX_STATIC);
  
  // If we are not given a type and cannot find one to begin inference, return.
  if (!t && is_void_type(v->t) && !is_store) return;

  // Assume we will be doing inference based on final output type.
  if (!t) {
    if (v->op == VAL_LD_IDX || v->op == VAL_ST_IDX) {
      t = &v->args[0]->t;
    } else if (is_static) {
      t = &v->static_arg->t;
    } else {
      t = &v->t;
    }
  }
  if (!is_store && is_void_type(v->t)) v->t = *t;

  if (v->op == VAL_CONCATENATE) {
    if (v->args.size() == 1)
      infer_type(v->args[0], t);
    // else do nothing for now. TODO: do something?
  } else if (v->op == VAL_PHI) {
    // To prevent infinite recursion, don't recurse on phis.
  } else if (v->op == VAL_AND_REDUCE || v->op == VAL_OR_REDUCE) {
    // Return type is always bit, input type unknowable.
    v->t = bit();
  } else if (is_store || v->op == VAL_LD_IDX || v->op == VAL_LD_IDX_STATIC) {
    unsigned size = *(t->type_vec.rbegin()), l2_size;
    for (l2_size = 0; (1ul << l2_size) < size; ++l2_size);
    type arg_type(u(l2_size));

    for (unsigned i = is_static?0:1; i != v->args.size(); ++i)
      infer_type(v->args[i], &arg_type);
  } else {
    // All other instructions are assumed to take inputs of type T and
    // produce an output of type T.
    for (auto &a : v->args) {
      infer_type(a, t);
    }
  }
}

void bscotch::finish_macro_env() {
  asm_prog_ptr->assemble_func();

  for (auto &f : asm_prog_ptr->p.functions) {
    for (auto &b : f.second.bbs) {
      for (auto &v : b->vals) {
        infer_type(v, NULL);
      }
    }
  }
  
  for (auto &p : vars) delete p;
  vars.clear();

  asm_prog_ptr = NULL;
}

bscotch::var::var(const type &t) {
  p = new varimpl(t);
}

bscotch::var::var() {
  p = new varimpl();
}

var &bscotch::var::operator=(const var &r) {
  // This should cost nothing in the final output.
  asm_prog_ptr->val(p->t, p->id, VAL_CONCATENATE).arg(r.p->id);
  p->v = asm_prog_ptr->v;

  return *this;
}

var bscotch::lit(const type &t, unsigned long val) {
  var r;

  asm_prog_ptr->val(u(64), r.p->id, VAL_CONST).const_arg(val);
  r.p->v = asm_prog_ptr->v;
  r.p->v->t = r.p->t = t;

  return r;
}

// Basic arithmetic/logic operators
static var bin_op(bscotch::if_op op, const var &a, const var &b) {
  var r(a.p->t);

  asm_prog_ptr->val(a.p->t, r.p->id, op).arg(a.p->id).arg(b.p->id);
  r.p->v = asm_prog_ptr->v;

  return r;
}

var bscotch::operator&(const var &a, const var &b)
  { return bin_op(VAL_AND, a, b); }

var bscotch::operator|(const var &a, const var &b)
  { return bin_op(VAL_OR, a, b); }

var bscotch::operator^(const var &a, const var &b)
  { return bin_op(VAL_XOR, a, b); }

var bscotch::operator+(const var &a, const var &b)
  { return bin_op(VAL_ADD, a, b); }

var bscotch::operator-(const var &a, const var &b)
  { return bin_op(VAL_SUB, a, b); }

var bscotch::operator*(const var &a, const var &b)
  { return bin_op(VAL_MUL, a, b); }

var bscotch::operator/(const var &a, const var &b)
  { return bin_op(VAL_DIV, a, b); }

// Basic unary operators
static var un_op(bscotch::if_op op, const var &a) {
  var r(op == VAL_OR_REDUCE || op == VAL_AND_REDUCE ? bit() : a.p->t);

  asm_prog_ptr->val(a.p->t, r.p->id, op).arg(a.p->id);
  r.p->v = asm_prog_ptr->v;

  return r;
}

var bscotch::operator!(const var &x) { return un_op(VAL_NOT, x); }
var bscotch::operator-(const var &x) { return un_op(VAL_NEG, x); }
var bscotch::operator~(const var &x) { return un_op(VAL_NOT, x); }
var bscotch::OrReduce(const var &x)  { return un_op(VAL_OR_REDUCE, x); }
var bscotch::AndReduce(const var &x) { return un_op(VAL_AND_REDUCE, x); }

// Basic comparison operators
var bscotch::operator==(const var &a, const var &b) { return !(a != b); }
var bscotch::operator!=(const var &a, const var &b) { return OrReduce(a ^ b); }

var bscotch::operator<(const var &a, const var &b) {
}

var bscotch::operator>=(const var &a, const var &b) {
}

var bscotch::operator>(const var &a, const var &b) {
}

var bscotch::operator<=(const var &a, const var &b) {
}

// Custom literal type. 1234_c is a var whose type will be inferred.
var bscotch::operator "" _c(const char *x) {
  var r;
  
  long long val = strtoll(x, NULL, 0);

  // Use void_type() to enable inference later.
  return lit(void_type(), val);
}

void bscotch::function(const char *name) {
  asm_prog_ptr->function(name);
}

void bscotch::label(const char *name) {
  asm_prog_ptr->label(name);
}

void bscotch::br(const char *dest) {
  asm_prog_ptr->br().target(dest);
}

argcollector<std::string> bscotch::br(const var &sel) {
  asm_prog_ptr->br(sel.p->id);
  return argcollector<string>(asm_prog_ptr->br_targets[asm_prog_ptr->b]);
}

argcollector<var> bscotch::spawn(const char *func) {
}

void bscotch::call() {
}

void bscotch::ret() {
  asm_prog_ptr->val(void_type(), varimpl::next_id++, VAL_CONST).const_arg(0);
  asm_prog_ptr->val(void_type(), varimpl::next_id++, VAL_RET);
}

void bscotch::ret(const var &rval) {
  un_op(VAL_RET, rval);
}

void bscotch::static_var(const char *name, const type &t) {
  asm_prog_ptr->static_var(t, name);
}

void bscotch::static_var(const char *name, const type &t, long initialval) {
  asm_prog_ptr->static_var(t, name, initialval);
}

static void check_static_var_existence(const char *name) {
  if (!asm_prog_ptr->f->static_vars.count(name)) {
    std::cerr << "Static var \"" << name << "\" not found." << endl;
    abort();
  }
}

var bscotch::load(const char *name) {
  check_static_var_existence(name);
  
  if_staticvar &v(asm_prog_ptr->f->static_vars[name]);
  type &t(v.t);
  var r(t);

  asm_prog_ptr->val(t, r.p->id, VAL_LD_STATIC).static_arg(name);

  return r;
}

var bscotch::load(const char *name, const var &idx) {
  check_static_var_existence(name);
  
  if_staticvar &v(asm_prog_ptr->f->static_vars[name]);
  type t(element_type(v.t));
  var r(t);

  asm_prog_ptr->val(t, r.p->id, VAL_LD_IDX_STATIC).
    static_arg(name).arg(idx.p->id);

  return r;
}

var bscotch::load(const var &in, const var &idx) {
  type &t(in.p->t);
  var r(t);
  
  asm_prog_ptr->val(t, r.p->id, VAL_LD_IDX).arg(in.p->id).arg(idx.p->id);

  return r;
}

var bscotch::load(const var &in, const var &idx, long len) {
  unsigned array_len(*(in.p->t.type_vec.rbegin())), l2_array_len;
  for (l2_array_len = 0; (1u<<l2_array_len) < array_len; ++l2_array_len);

  type t(in.p->t);
  *(t.type_vec.rbegin()) = len;
  var r(t);

  cout << "Selecting " << len << " elements from a " << array_len << " element value." << endl;

  asm_prog_ptr->val(t, r.p->id, VAL_LD_IDX).
    arg(in.p->id).arg(idx.p->id).arg(lit(u(l2_array_len), len).p->id);

  return r;
}

var bscotch::load(const var &in, const char *field) {
  type &t(in.p->t);
  unsigned field_idx, i;
  bool found = false;
  
  for (i = 0, field_idx = 0; i < t.type_vec.size(); i++) {
    if (t.field_name.count(i) ) {
      if (t.field_name[i] == field) { found = true; break; }
      else field_idx++;
    }
  }

  cout << "Found field \"" << field << "\" in struct at index " << field_idx
       << '.' << endl;
  
  if (!found) {
    cerr << "Field \"" << field << "\" not found in struct." << endl;
    abort();
  }
  
  return load(in, lit(u(32), field_idx));
}

void bscotch::store(const char *name, const var &d) {
  check_static_var_existence(name);

  if_staticvar &v(asm_prog_ptr->f->static_vars[name]);
  type &t(v.t);

  asm_prog_ptr->val(varimpl::next_id++, VAL_ST_STATIC)
    .static_arg(name).arg(d.p->id);
}

void bscotch::store(const char *name, const var &idx, const var &d) {
  check_static_var_existence(name);

  if_staticvar &v(asm_prog_ptr->f->static_vars[name]);
  unsigned array_len(*(v.t.type_vec.rbegin())), l2_array_len;
  for (l2_array_len = 0; (1u<<l2_array_len) < array_len; ++l2_array_len);

  asm_prog_ptr->val(void_type(), varimpl::next_id++, VAL_ST_IDX_STATIC).
    static_arg(name).arg(idx.p->id).arg(d.p->id);
}

void bscotch::store(const char *name, const char *field, const var &d) {
}

var bscotch::repl(const var &in, const var &idx, const var &d) {
  type &t(in.p->t);
  var r(t);
  
  asm_prog_ptr->val(t, r.p->id, VAL_ST_IDX).arg(in.p->id)
    .arg(idx.p->id).arg(d.p->id);

  return r;
}

var bscotch::repl(const var &in, const var &idx, const var &d, unsigned len) {
}

var bscotch::repl(const var &in, const char *field, const var &d) {
  type &t(in.p->t);
  unsigned field_idx, i;
  bool found = false;
  
  for (i = 0, field_idx = 0; i < t.type_vec.size(); i++) {
    if (t.field_name.count(i) ) {
      if (t.field_name[i] == field) { found = true; break; }
      else field_idx++;
    }
  }

  cout << "Found field \"" << field << "\" in struct at index " << field_idx
       << '.' << endl;
  
  if (!found) {
    cerr << "Field \"" << field << "\" not found in struct." << endl;
    abort();
  }

  return repl(in, lit(u(32), field_idx), d);
}

var bscotch::repl(const var &in, const char *field, const var &d, unsigned len)
{
}
