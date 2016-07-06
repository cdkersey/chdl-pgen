#ifndef BSCOTCH_TYPE_H
#define BSCOTCH_TYPE_H

#include <iostream>

#include <vector>
#include <string>
#include <sstream>
#include <map>

namespace bscotch {
  // Types are represented by vectors of integers.
  enum type_part {
    TYPE_BIT = -100,
    TYPE_S = -101,             // Followed by #bits
    TYPE_U = -102,             // Followed by #bits
    TYPE_ARRAY = -1000,        // Followed by dimension, follows element type.
    TYPE_STATIC_ARRAY = -1001, // Followed by dimension, follows element type.
    TYPE_STRUCT_BEGIN = -1002, // Followed by #elements.
    TYPE_FIELD_DELIM = -1003   // Next element of struct.
  };
  
  struct type {
    std::vector<int> type_vec;
    std::map<int, std::string> field_name;

    std::string str(int start = 0, int &chars = *((int*)NULL));
    unsigned size(int start = 0, int &chars = *((int*)NULL));
  };

  type element_type(type t);
  bool is_sram_array(const type &t);
  unsigned array_len(const type &t);
  bool is_struct(const type &t);

  // Simple names for common types.
  type u(unsigned nbits);
  type s(unsigned nbits);
  
  void print(std::ostream &out, bscotch::type &t);
  std::string type_chdl(const type &t, int s = 0, int &end = *(int*)NULL);
  std::string type_cpp(const type &t, std::string name,
                       int s = 0, int &end = *(int*)NULL);

  std::string field_name_by_idx(type &t, int idx, int s = 0);
}
#endif
