#ifndef BSCOTCH_TYPE_H
#define BSCOTCH_TYPE_H

#include <iostream>

#include <vector>
#include <string>
#include <sstream>

namespace bscotch {
  // Types are represented by vectors of integers.
  enum type_part {
    TYPE_BIT = -100,
    TYPE_S = -101,             // Followed by #bits
    TYPE_U = -102,             // Followed by #bits
    TYPE_ARRAY = -1000,        // Followed by dimension, then array type.
    TYPE_STATIC_ARRAY = -1001, // Followed by dimension, then array type.
    TYPE_STRUCT_BEGIN = -1002, // Followed by #elements.
    TYPE_FIELD_DELIM = -1003   // Next element of struct.
  };

  // TODO: add operations on types.
  struct type {
    std::vector<int> type_vec;
    std::string str(int start = 0);
  };
};

std::string bscotch::type::str(int s) {
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
      for (unsigned j = 0; j < fields; ++i) {
	if (j != 0) oss << ", ";
	oss << str(i);
	while (type_vec[++i] != TYPE_FIELD_DELIM);
      }
      oss << '}';
    }
  }

  return oss.str();
}

void print(std::ostream &out, bscotch::type &t) {
  using namespace std;

  if (t.type_vec.size() == 0)
    out << "void";
  else
    out << t.str();
}

#endif
