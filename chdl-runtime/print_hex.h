#include <iostream>
#include <iomanip>

#include <chdl/chdl.h>
#include <chdl/ag.h>
#include <chdl/egress.h>

namespace chdl {
  template <typename T> using print_hex_ret_t = flit<ag<STP("live"), T, ag<STP("rval"), bvec<0> > > >;
  template <typename T> using print_hex_call_t = flit<ag<STP("live"), T, ag<STP("arg0"), ui<32> > > >;

  template <typename OPAQUE>
    void print_hex(
      print_hex_ret_t <OPAQUE> &ret,
      print_hex_call_t<OPAQUE> &call
    )
  {
    using namespace std;
    unsigned *val = new unsigned();
    
    _(call, "ready") = Lit(1);
    node printhex(_(call, "valid"));
    EgressInt(*val, _(_(call, "contents"), "arg0"));
    EgressFunc([val](bool valid) { if (valid) cout << "printhex> " << setw(8) << setfill('0') << hex << *val << dec << endl; }, _(call, "valid")); 
  }
}
