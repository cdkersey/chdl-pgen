#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <map>

#include "type.h"

unsigned bscotch::type::size(int s, int &end) {
  using namespace bscotch;
  using namespace std;

  unsigned sz = 0, i;

  for (i = s; i < type_vec.size() && type_vec[i] != TYPE_FIELD_DELIM; ++i) {
    if (type_vec[i] == TYPE_BIT) {
      sz += 1;
    } else if (type_vec[i] == TYPE_S || type_vec[i] == TYPE_U) {
      sz += type_vec[++i];
    } else if (type_vec[i] == TYPE_ARRAY) {
      /* sz *= type_vec[++i]; TODO: what's the "size" of an SRAM? */
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

std::string bscotch::type_chdl(bscotch::type &t, int s, int &end)
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
            oss << "STP(\"" << t.field_name[i] << "\"), ";
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

std::string bscotch::type_cpp
  (bscotch::type &t, std::string name, int s, int &end)
{
  using namespace bscotch;
  using namespace std;

  ostringstream oss;
  unsigned i;

  for (i = s; i < t.type_vec.size() && t.type_vec[i] != TYPE_FIELD_DELIM; ++i) {
    if (t.type_vec[i] == TYPE_BIT) {
      oss << "bool " << name;
    } else if (t.type_vec[i] == TYPE_S) {
      oss << "si<" << t.type_vec[++i] << "> " << name;
    } else if (t.type_vec[i] == TYPE_U) {
      oss << "ui<" << t.type_vec[++i] << "> " << name;
    } else if (t.type_vec[i] == TYPE_ARRAY) {
      // TODO: Are SRAM arrays best represented as arrays?
      string s = oss.str();
      oss = ostringstream();
      oss << s << '[' << t.type_vec[++i] << ']';
    } else if (t.type_vec[i] == TYPE_STATIC_ARRAY) {
      string s = oss.str();
      oss = ostringstream();
      oss << s << '[' << t.type_vec[++i] << ']';
    } else if (t.type_vec[i] == TYPE_STRUCT_BEGIN) {
      unsigned fields = t.type_vec[++i];
      if (fields == 0) {
	oss << "void";
      } else {
        oss << "struct {";
        for (unsigned j = 0; j < fields; ++j) {
          ++i;
          int pos;
          oss << type_cpp(t, t.field_name[i], i, pos) << ';';
          i = pos;
        }
	oss << "} " << name;
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