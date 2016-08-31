#include "asm-macro.h"

#include <cstdlib>

using namespace pgen;
using namespace std;

static asm_prog *asm_prog_ptr;
vector<pgen::varimpl *> vars;

struct pgen::varimpl {
  varimpl(): v(NULL), id(next_id++), generic(true) {
    vars.push_back(this);
  }
  
  varimpl(const type &t): t(t), v(NULL), id(next_id++), generic(false) {
    vars.push_back(this);
  }

  pgen::asm_prog::val_id_t id;
  if_val *v; // Most recently assigned value. 
  type t; // Type.
  bool generic; // Type not yet assigned.

  static pgen::asm_prog::val_id_t next_id;
};

pgen::asm_prog::val_id_t pgen::varimpl::next_id;

// Initialize the macro API. This is a thread-unsafe, stateful API designed as
// a stop-gap solution between the assembler and a language front-end/parser.
void pgen::init_macro_env(asm_prog &a) {
  asm_prog_ptr = &a;
  pgen::varimpl::next_id = 0;
  for (auto &p : vars) delete p;
  vars.clear();
}

void pgen::finish_macro_env() {
  asm_prog_ptr->assemble_func();

  for (auto &p : vars) delete p;
  vars.clear();

  asm_prog_ptr = NULL;
}

pgen::var::var(const type &t) {
  p = new varimpl(t);
}

pgen::var::var() {
  p = new varimpl();
}

var &pgen::var::operator=(const var &r) {
  // This should cost nothing in the final output.
  asm_prog_ptr->val(p->t, p->id, VAL_CONCATENATE).arg(r.p->id);
  p->v = asm_prog_ptr->v;

  return *this;
}

var pgen::lit(const type &t, unsigned long val) {
  var r;

  asm_prog_ptr->val(u(64), r.p->id, VAL_CONST).const_arg(val);
  r.p->v = asm_prog_ptr->v;
  r.p->v->t = r.p->t = t;

  return r;
}

var pgen::arg(const type &t) {
  var r;

  asm_prog_ptr->val(t, r.p->id, VAL_ARG);
  r.p->v = asm_prog_ptr->v;
  r.p->v->t = r.p->t = t;

  return r;
}

// Basic arithmetic/logic operators
static var bin_op(pgen::if_op op, const var &a, const var &b) {
  var r(op == VAL_EQ || op == VAL_LT ? bit() : a.p->t);

  asm_prog_ptr->val(r.p->t, r.p->id, op).arg(a.p->id).arg(b.p->id);
  r.p->v = asm_prog_ptr->v;

  return r;
}

var pgen::operator&(const var &a, const var &b)
  { return bin_op(VAL_AND, a, b); }

var pgen::operator|(const var &a, const var &b)
  { return bin_op(VAL_OR, a, b); }

var pgen::operator^(const var &a, const var &b)
  { return bin_op(VAL_XOR, a, b); }

var pgen::operator+(const var &a, const var &b)
  { return bin_op(VAL_ADD, a, b); }

var pgen::operator-(const var &a, const var &b)
  { return bin_op(VAL_SUB, a, b); }

var pgen::operator*(const var &a, const var &b)
  { return bin_op(VAL_MUL, a, b); }

var pgen::operator/(const var &a, const var &b)
  { return bin_op(VAL_DIV, a, b); }

// Basic unary operators
static var un_op(pgen::if_op op, const var &a) {
  type t;
  if (op == VAL_OR_REDUCE || op == VAL_AND_REDUCE)
    t = bit();
  else if (op == VAL_RET)
    t = void_type();
  else
    t = a.p->t;
  var r(t);

  asm_prog_ptr->val(t, r.p->id, op).arg(a.p->id);
  r.p->v = asm_prog_ptr->v;

  return r;
}

var pgen::operator!(const var &x) { return un_op(VAL_NOT, x); }
var pgen::operator-(const var &x) { return un_op(VAL_NEG, x); }
var pgen::operator~(const var &x) { return un_op(VAL_NOT, x); }
var pgen::or_reduce(const var &x)  { return un_op(VAL_OR_REDUCE, x); }
var pgen::and_reduce(const var &x) { return un_op(VAL_AND_REDUCE, x); }

// Basic comparison operators
var pgen::operator==(const var &a, const var &b) {
  return bin_op(VAL_EQ, a, b);
}

var pgen::operator!=(const var &a, const var &b) {
  return !bin_op(VAL_EQ, a, b);
}

var pgen::operator<(const var &a, const var &b) {
  return bin_op(VAL_LT, a, b);
}

var pgen::operator>=(const var &a, const var &b) {
  return !bin_op(VAL_LT, a, b);
}

var pgen::operator>(const var &a, const var &b) {
  return bin_op(VAL_LT, b, a);
}

var pgen::operator<=(const var &a, const var &b) {
  return !bin_op(VAL_LT, b, a);
}

void pgen::function(const char *name) {
  asm_prog_ptr->function(name);
}

void pgen::label(const char *name) {
  asm_prog_ptr->label(name);
}

void pgen::br(const char *dest) {
  asm_prog_ptr->br().target(dest);
}

argcollector<std::string> pgen::br(const var &sel) {
  asm_prog_ptr->br(sel.p->id);
  return argcollector<string>(
    [](std::string x){
      asm_prog_ptr->br_targets[asm_prog_ptr->b].push_back(list<string>(1, x));
    }
  );
}

argcollector<std::string> pgen::br_spawn() {
  // If we do not call br(), we enable the use of br_spawn() to declare spawn
  // groups.
  // asm_prog_ptr->br();

  asm_prog_ptr->br_targets[asm_prog_ptr->b].push_back(list<string>());

  return argcollector<string>(
    [](std::string x){
      asm_prog_ptr->br_targets[asm_prog_ptr->b].rbegin()->push_back(x);
    }
  );
}

argcollector<var> pgen::spawn(const char *func) {
  type t(void_type());

  var r;
  asm_prog_ptr->val(t, r.p->id, VAL_SPAWN).func_arg(func);
  r.p->v = asm_prog_ptr->v;
   
  return argcollector<var>([](var v){ asm_prog_ptr->arg(v.p->id); });
}

void pgen::stall(const var &v) { asm_prog_ptr->stall(v.p->id); }

argcollector<var> pgen::call(const char *func) {
  type t(void_type());

  var r;
  asm_prog_ptr->val(t, r.p->id, VAL_CALL).func_arg(func);
  r.p->v = asm_prog_ptr->v;
   
  return argcollector<var>([](var v){ asm_prog_ptr->arg(v.p->id); });
}

argcollector<var> pgen::call(const char *func, const var &r) {
  type t(r.p->t);

  asm_prog_ptr->val(t, r.p->id, VAL_CALL).func_arg(func);
  r.p->v = asm_prog_ptr->v;
   
  return argcollector<var>([](var v){ asm_prog_ptr->arg(v.p->id); });
}

argcollector<var> pgen::sel(const var &out, const var &s) {
  
  asm_prog_ptr->val(out.p->t, out.p->id, VAL_SELECT);
  out.p->v = asm_prog_ptr->v;

  
  if_val *vp = asm_prog_ptr->v;

  asm_prog_ptr->arg_ids[vp].push_back(s.p->id);
  
  return argcollector<var>(
    [vp](var v){ asm_prog_ptr->arg_ids[vp].push_back(v.p->id); }
  );
}

argcollector<var> pgen::cat(const var &r) {
  asm_prog_ptr->val(r.p->t, r.p->id, VAL_CONCATENATE);
  r.p->v = asm_prog_ptr->v;

  if_val *vp = asm_prog_ptr->v;
  return argcollector<var>(
    [vp](var v){ asm_prog_ptr->arg_ids[vp].push_back(v.p->id); }
  );
}

argcollector<var> pgen::build(const var &r) {
  asm_prog_ptr->val(r.p->t, r.p->id, VAL_BUILD);
  r.p->v = asm_prog_ptr->v;

  if_val *vp = asm_prog_ptr->v;
  return argcollector<var>(
    [vp](var v){ asm_prog_ptr->arg_ids[vp].push_back(v.p->id); }
  );
}

void pgen::ret() {
  asm_prog_ptr->val(void_type(), varimpl::next_id++, VAL_RET);
}

void pgen::ret(const var &rval) {
  un_op(VAL_RET, rval);
  asm_prog_ptr->f->rtype = rval.p->t;
}

void pgen::static_var(const char *name, const type &t) {
  asm_prog_ptr->static_var(t, name);
}

void pgen::static_var(const char *name, const type &t, long initialval) {
  asm_prog_ptr->static_var(t, name, initialval);
}

void pgen::bcast_var(const char *name, const type &t) {
  asm_prog_ptr->bcast_var(t, name);
}

static void check_static_var_existence(const char *name) {
  if (!asm_prog_ptr->f->static_vars.count(name)) {
    std::cerr << "Static var \"" << name << "\" not found." << endl;
    abort();
  }
}

unsigned var_const_val(const var &v) {
  return const_val(*v.p->v);
}

var pgen::load(const char *name) {
  check_static_var_existence(name);

  if_staticvar &v(asm_prog_ptr->f->static_vars[name]);
  type &t(v.t);
  var r(t);

  asm_prog_ptr->val(t, r.p->id, VAL_LD_STATIC).static_arg(name);

  return r;
}

var pgen::bcast_valid(const char *name) {
  check_static_var_existence(name);

  if_staticvar &v(asm_prog_ptr->f->static_vars[name]);

  var r(bit());
  if_op op = VAL_BCAST_VALID_STATIC;
  asm_prog_ptr->val(bit(), r.p->id, op).static_arg(name);

  return r;
}

var pgen::load(const char *name, const var &idx) {
  check_static_var_existence(name);

  if_staticvar &v(asm_prog_ptr->f->static_vars[name]);
  type t;

  if (is_struct(v.t))
    t = v.t.get_field_type(var_const_val(idx));
  else if (is_sram_array(v.t) || is_static_array(v.t))
    t = element_type(v.t);
  else
    t = bit();
 
  var r(t);

  asm_prog_ptr->val(t, r.p->id, VAL_LD_IDX_STATIC).
    static_arg(name).arg(idx.p->id);

  return r;
}

var pgen::load(const var &in, const var &idx) {
  type t;

  if (is_struct(in.p->t))
    t = in.p->t.get_field_type(var_const_val(idx));
  else if (is_sram_array(in.p->t) || is_static_array(in.p->t))
    t = element_type(in.p->t);
  else
    t = bit();

  var r(t);
  
  asm_prog_ptr->val(t, r.p->id, VAL_LD_IDX).arg(in.p->id).arg(idx.p->id);

  return r;
}

var pgen::load(const var &in, const var &idx, const var &len) {
  type t(in.p->t);
  *(t.type_vec.rbegin()) = var_const_val(len);
  var r(t);

  asm_prog_ptr->val(t, r.p->id, VAL_LD_IDX).
    arg(in.p->id).arg(idx.p->id).arg(len.p->id);

  return r;
}

var pgen::load(const var &in, const var &idx, unsigned len) {
  return load(in, idx, lit(u(32), len));
}

var pgen::load(const var &in, const char *field) {
  type &t(in.p->t);
  unsigned field_idx = t.get_field_idx(field);
  type field_type = t.get_field_type(field_idx);

  if (field_idx == -1) {
    cerr << "Field \"" << field << "\" not found in struct." << endl;
    abort();
  }
  
  var r(field_type);

  r = load(in, lit(u(32), field_idx));

  return r;
}

void pgen::store(const char *name, const var &d) {
  check_static_var_existence(name);

  if_staticvar &v(asm_prog_ptr->f->static_vars[name]);
  type &t(v.t);

  asm_prog_ptr->val(varimpl::next_id++, VAL_ST_STATIC)
    .static_arg(name).arg(d.p->id);
}

void pgen::store(const char *name, const var &idx, const var &d) {
  check_static_var_existence(name);

  if_staticvar &v(asm_prog_ptr->f->static_vars[name]);
  
  unsigned array_len(*(v.t.type_vec.rbegin())), l2_array_len;
  for (l2_array_len = 0; (1u<<l2_array_len) < array_len; ++l2_array_len);

  asm_prog_ptr->val(void_type(), varimpl::next_id++, VAL_ST_IDX_STATIC)
    .static_arg(name).arg(idx.p->id).arg(d.p->id);
}

void pgen::store(const char *name, const char *field, const var &d) {
  check_static_var_existence(name);

  if_staticvar &v(asm_prog_ptr->f->static_vars[name]);
  
  type &t(v.t);
  unsigned field_idx = t.get_field_idx(field);
  type field_type = t.get_field_type(field_idx);

  if (field_idx == -1) {
    cerr << "Field \"" << field << "\" not found in struct." << endl;
    abort();
  }
  
  store(name, lit(u(32), field_idx), d);
}

var pgen::repl(const var &in, const var &idx, const var &d) {
  type &t(in.p->t);
  var r(t);
  
  asm_prog_ptr->val(t, r.p->id, VAL_ST_IDX).arg(in.p->id)
    .arg(idx.p->id).arg(d.p->id);

  return r;
}

var pgen::repl(const var &in, const var &idx, const var &d, const var &len) {
  type &t(in.p->t);
  var r(t);

  asm_prog_ptr->val(t, r.p->id, VAL_ST_IDX).arg(in.p->id)
    .arg(idx.p->id).arg(len.p->id).arg(d.p->id);

  return r;
}

var pgen::repl(const var &in, const var &idx, const var &d, unsigned len) {
  return repl(in, idx, d, lit(u(32), len));
}

var pgen::repl(const var &in, const char *field, const var &d) {
  type &t(in.p->t);
  unsigned field_idx, i;
  bool found = false;
  
  for (i = 0, field_idx = 0; i < t.type_vec.size(); i++) {
    if (t.field_name.count(i) ) {
      if (t.field_name[i] == field) { found = true; break; }
      else field_idx++;
    }
  }

  if (!found) {
    cerr << "Field \"" << field << "\" not found in struct." << endl;
    abort();
  }

  return repl(in, lit(u(32), field_idx), d);
}

void pgen::pred(const var &p) {
  asm_prog_ptr->pred(p.p->id);
}
