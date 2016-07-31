#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <map>

#include "type.h"

int bscotch::type::compare(const type &x) const {
  using std::string;
  
  if (x.type_vec.size() < type_vec.size()) return -1;
  else if (x.type_vec.size() > type_vec.size()) return 1;

  // Type vec lengths are equal.
  for (unsigned i = 0; i < type_vec.size(); ++i) {
    if (x.type_vec[i] < type_vec[i]) return -1;
    else if (x.type_vec[i] > type_vec[i]) return 1;
    else if (field_name.count(i)) {
      if (!x.field_name.count(i)) return -1;
      else {
        const string &xname(x.field_name.find(i)->second),
          &name(field_name.find(i)->second);
        if (xname.length() < name.length()) return -1;
        else if (xname.length() > name.length()) return 1;
        else {
        for (unsigned j = 0; j < name.length(); ++j) {
          if (xname[j] < name[j]) return -1;
          else if (xname[j] > name[j]) return 1;
        }
        }
      }
    }
  }

  // Types are equal.
  return 0;
}

bool bscotch::type::operator==(const type &x) const { return (compare(x) == 0); }
bool bscotch::type::operator>(const type &x) const {  return (compare(x) > 0); }
bool bscotch::type::operator<(const type &x) const {  return (compare(x) < 0); }
bool bscotch::type::operator<=(const type &x) const { return (compare(x) <= 0); }
bool bscotch::type::operator>=(const type &x) const { return (compare(x) >= 0); }

void bscotch::type::get_field_start_end(int &start, int &end, int idx) const {
  int l = 0, fields = 0, cur_field = 0;
  bool field_start = false;
  for (unsigned i = 0; i < type_vec.size(); ++i) {
    if (field_start) {
      if (cur_field == idx) {
        start = i;
      } else if (cur_field == idx + 1) {
        end = i;
        return;
      }
      field_start = false;
    }
 
    int p = type_vec[i];
    if (p == TYPE_STRUCT_BEGIN) {
      ++l;
      fields = type_vec[++i];
      if (l == 1) field_start = true;
    } else if (p == TYPE_FIELD_DELIM) {
      if (--fields == 0) {
        --l;
      } else {
        if (l == 1) {
          ++cur_field;
          field_start = true;
        }
      }
    } else if (p == TYPE_ARRAY) {
      ++i;
    }
  }

  end = type_vec.size() - 1;
}

std::string bscotch::type::get_field_name(int idx) const {
  int start, end;
  get_field_start_end(start, end, idx);

  return field_name.find(start)->second;
}

int bscotch::type::get_field_idx(std::string name) const {
  int l = 0, fields = 0, cur_field = 0;
  bool field_start = false;
  for (unsigned i = 0; i < type_vec.size(); ++i) {
    if (field_start) {
      if (field_name.count(i) && field_name.find(i)->second == name)
        return cur_field;
      field_start = false;
    }
 
    int p = type_vec[i];
    if (p == TYPE_STRUCT_BEGIN) {
      ++l;
      fields = type_vec[++i];
      if (l == 1) field_start = true;
    } else if (p == TYPE_FIELD_DELIM) {
      if (--fields == 0) {
        --l;
      } else {
        if (l == 1) {
          ++cur_field;
          field_start = true;
        }
      }
    } else if (p == TYPE_ARRAY) {
      ++i;
    }
  }

  return -1;
}

bscotch::type bscotch::type::get_field_type(int idx) const {
  type r;
  
  int start, end;
  get_field_start_end(start, end, idx);

  for (unsigned i = start; i < end; ++i) {
    if (field_name.count(i))
      r.field_name[i - start] = field_name.find(i)->second;
    r.type_vec.push_back(type_vec[i]);
  }

  return r;
}

int bscotch::type::get_n_fields() const { return type_vec[1]; }

bscotch::type &bscotch::type::add_field(std::string n, const bscotch::type &t) {
  if (type_vec[0] != TYPE_STRUCT_BEGIN) {
    std::cerr << "Attempt to add field \"" << n << "\" to non-struct type."
              << std::endl;
    abort();
  }

  bool need_delim(type_vec[1] != 0);
  
  type_vec[1]++;

  if (!need_delim) type_vec.pop_back();
  unsigned origin = type_vec.size();
  field_name[origin] = n;

  for (unsigned i = 0; i < t.type_vec.size(); ++i) {
    type_vec.push_back(t.type_vec[i]);
    if (t.field_name.count(i))
      field_name[origin + i] = t.field_name.find(i)->second;
  }

  type_vec.push_back(TYPE_FIELD_DELIM);
  
  return *this;
}

unsigned bscotch::type::size(int s, int &end) const {
  using namespace bscotch;
  using namespace std;

  unsigned sz = 0, i;

  for (i = s; i < type_vec.size() && type_vec[i] != TYPE_FIELD_DELIM; ++i) {
    if (type_vec[i] == TYPE_BIT) {
      sz += 1;
    } else if (type_vec[i] == TYPE_S || type_vec[i] == TYPE_U) {
      sz += type_vec[++i];
    } else if (type_vec[i] == TYPE_ARRAY) {
      sz *= type_vec[++i]; /* TODO: what's the "size" of an SRAM? */
    } else if (type_vec[i] == TYPE_STATIC_ARRAY) {
      sz *= type_vec[++i];
    } else if (type_vec[i] == TYPE_STRUCT_BEGIN) {
      unsigned fields = type_vec[++i];
      for (unsigned j = 0; j < fields; ++j) {
        ++i;
        int pos;
        sz += size(i, pos);
        i = pos;
      }
    }
  }

  if (&end) end = i;
  
  return sz;
}

bscotch::type bscotch::void_type() {
  type t;
  return t;
}

bscotch::type bscotch::bit() {
  type t;
  t.type_vec.push_back(TYPE_BIT);
  return t;
}

bscotch::type bscotch::u(unsigned nbits) {
  type t;
  t.type_vec.push_back(TYPE_U);
  t.type_vec.push_back(nbits);

  return t;
}

bscotch::type bscotch::s(unsigned nbits) {
  type t;
  t.type_vec.push_back(TYPE_S);
  t.type_vec.push_back(nbits);

  return t;
}

bscotch::type bscotch::struct_type() {
  type t;
  t.type_vec.push_back(TYPE_STRUCT_BEGIN);
  t.type_vec.push_back(0);
  t.type_vec.push_back(TYPE_FIELD_DELIM);

  return t;
}

bscotch::type bscotch::a(bscotch::type t, unsigned len) {
  t.type_vec.push_back(TYPE_ARRAY);
  t.type_vec.push_back(len);
  
  return t;
}

bscotch::type bscotch::sa(bscotch::type t, unsigned len) {
  t.type_vec.push_back(TYPE_STATIC_ARRAY);
  t.type_vec.push_back(len);
  
  return t;
}

bscotch::type bscotch::element_type(bscotch::type t) {
  unsigned n = t.type_vec.size();
  t.type_vec.resize(n - 2);

  return t;
}

unsigned bscotch::array_len(const bscotch::type &t) {
  unsigned n = t.type_vec.size();
  return t.type_vec[n - 1];
}

bool bscotch::is_sram_array(const bscotch::type &t) {
  unsigned n = t.type_vec.size();
  return t.type_vec.size() >= 2 && t.type_vec[n - 2] == TYPE_ARRAY;
}

bool bscotch::is_static_array(const type &t) {
  unsigned n = t.type_vec.size();
  return t.type_vec.size() >= 2 && t.type_vec[n - 2] == TYPE_STATIC_ARRAY;
}

bool bscotch::is_integer_type(const type &t) {
  return t.type_vec.size() >= 1 &&
    (t.type_vec[0] == TYPE_S || t.type_vec[0] == TYPE_U);
}

bool bscotch::is_bit_type(const type &t) {
  return t.type_vec.size() >= 1 && t.type_vec[0] == TYPE_BIT;
}

bool bscotch::is_struct(const bscotch::type &t) {
  return t.type_vec.size() >= 1 && t.type_vec[0] == TYPE_STRUCT_BEGIN;
}

bool bscotch::is_void_type(const type &t) { return t.type_vec.size() == 0; }

std::string bscotch::type::str(int s, int &end) {
  using namespace bscotch;
  using namespace std;

  ostringstream oss;
  unsigned i;

  for (i = s; i < type_vec.size() && type_vec[i] != TYPE_FIELD_DELIM; ++i) {
    if (type_vec[i] == TYPE_BIT) {
      oss << "bit";
    } else if (type_vec[i] == TYPE_S) {
      oss << "s" << type_vec[++i];
    } else if (type_vec[i] == TYPE_U) {
      oss << "u" << type_vec[++i];
    } else if (type_vec[i] == TYPE_ARRAY) {
      oss << '[' << type_vec[++i] << ']';
    } else if (type_vec[i] == TYPE_STATIC_ARRAY) {
      oss << "[[" << type_vec[++i] << "]]";
    } else if (type_vec[i] == TYPE_STRUCT_BEGIN) {
      unsigned fields = type_vec[++i];
      oss << '{';
      for (unsigned j = 0; j < fields; ++j) {
	if (j != 0) oss << ", ";
	++i;
	if (field_name.count(i)) oss << field_name[i] << " : ";
	int pos;
	oss << str(i, pos);
	i = pos;
      }
      oss << '}';
    }
  }

  if (i == s) oss << "void";
  
  if (&end) end = i;
  
  return oss.str();
}

std::string bscotch::type_chdl(const bscotch::type &t, int s, int &end)
{
  using namespace bscotch;
  using namespace std;

  ostringstream oss;
  unsigned i;

  for (i = s; i < t.type_vec.size() && t.type_vec[i] != TYPE_FIELD_DELIM; ++i) {
    if (t.type_vec[i] == TYPE_BIT) {
      oss << "node";
    } else if (t.type_vec[i] == TYPE_S) {
      oss << "si<" << t.type_vec[++i] << '>';
    } else if (t.type_vec[i] == TYPE_U) {
      oss << "ui<" << t.type_vec[++i] << '>';
    } else if (t.type_vec[i] == TYPE_ARRAY) {
      // TODO: Are SRAM arrays best represented as vecs?
      string s = oss.str();
      oss = ostringstream();
      oss << "vec<" << t.type_vec[++i] << ", " << s << " >";
    } else if (t.type_vec[i] == TYPE_STATIC_ARRAY) {
      string s = oss.str();
      oss = ostringstream();
      oss << "vec<" << t.type_vec[++i] << ", " << s << " >";
    } else if (t.type_vec[i] == TYPE_STRUCT_BEGIN) {
      unsigned fields = t.type_vec[++i];
      if (fields == 0) {
	oss << "chdl_void";
      } else {
        oss << "ag<";
        for (unsigned j = 0; j < fields; ++j) {
          if (j != 0) oss << ", ag<";
          ++i;
          if (t.field_name.count(i))
            oss << "STP(\"" << t.field_name.find(i)->second << "\"), ";
          int pos;
          oss << type_chdl(t, i, pos);
          i = pos;
        }
        for (unsigned i = 0; i < fields; ++i) oss << " >";
      }
    }
  }

  if (i == s) oss << "chdl_void";
  
  if (&end) end = i;
  
  return oss.str();
}

std::string bscotch::type_cpp(const bscotch::type &t, int s, int &end) {
  using namespace bscotch;
  using namespace std;

  ostringstream oss;
  unsigned i;

  for (i = s; i < t.type_vec.size() && t.type_vec[i] != TYPE_FIELD_DELIM; ++i) {
    if (t.type_vec[i] == TYPE_BIT) {
      oss << "bool";
    } else if (t.type_vec[i] == TYPE_S) {
      oss << "si<" << t.type_vec[++i] << "> ";
    } else if (t.type_vec[i] == TYPE_U) {
      oss << "ui<" << t.type_vec[++i] << "> ";
    } else if (t.type_vec[i] == TYPE_ARRAY) {
      // TODO: Are SRAM arrays best represented as arrays?
      string s = oss.str();
      oss = ostringstream();
      oss << "array<" << t.type_vec[++i] << ", " << s << '>';
    } else if (t.type_vec[i] == TYPE_STATIC_ARRAY) {
      string s = oss.str();
      oss = ostringstream();
      oss << "array<" << t.type_vec[++i] << ", " << s << '>';
    } else if (t.type_vec[i] == TYPE_STRUCT_BEGIN) {
      unsigned fields = t.type_vec[++i];
      if (fields == 0) {
	oss << "void";
      } else {
        oss << "struct {";
        for (unsigned j = 0; j < fields; ++j) {
          ++i;
          int pos;
          oss << type_cpp(t, i, pos)
              << ' ' << t.field_name.find(i)->second << ';';
          i = pos;
        }
	oss << "} ";
      }
    }
  }

  if (i == s) oss << "void";
  
  if (&end) end = i;

  return oss.str();
}

void bscotch::print(std::ostream &out, bscotch::type &t) {
  using namespace std;

  if (t.type_vec.size() == 0)
    out << "void";
  else
    out << t.str();
}
