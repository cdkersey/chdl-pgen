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

    type &add_field(std::string, const type &t);

    void get_field_start_end(int &start, int &end, int idx);
    std::string get_field_name(int idx);
    int get_field_idx(std::string name);
    type get_field_type(int idx);
    
    std::string str(int start = 0, int &chars = *((int*)NULL));
    unsigned size(int start = 0, int &chars = *((int*)NULL)) const;
  };

  type element_type(type t);
  bool is_sram_array(const type &t);
  bool is_static_array(const type &t);
  bool is_integer_type(const type &t);
  unsigned array_len(const type &t);
  bool is_struct(const type &t);
  bool is_void_type(const type &t);

  // Simple names for common types.
  type u(unsigned nbits);
  type s(unsigned nbits);
  type a(type t, unsigned len);
  type sa(type t, unsigned len);
  type bit();
  type struct_type();
  type void_type();
  
  void print(std::ostream &out, bscotch::type &t);
  std::string type_chdl(const type &t, int s = 0, int &end = *(int*)NULL);
  std::string type_cpp(const type &t, int s = 0, int &end = *(int*)NULL);
}
#endif
