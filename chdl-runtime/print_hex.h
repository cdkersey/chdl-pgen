#include <iostream>
#include <iomanip>

#include <chdl/chdl.h>
#include <chdl/ag.h>
#include <chdl/egress.h>

namespace chdl {
  template <typename T> using print_hex_ret_t =
    flit<ag<STP("live"), T, ag<STP("rval"), bvec<0> > > >;
  template <typename T> using print_hex_call_t =
    flit<ag<STP("live"), T, ag<STP("arg0"), ui<32> > > >;

  template <typename OPAQUE, typename G>
    void print_hex(
      print_hex_ret_t <OPAQUE> &ret,
      print_hex_call_t<OPAQUE> &call,
      G &globals
    )
  {
    using namespace std;
    unsigned *val = new unsigned();
    
    _(ret, "valid") = _(call, "valid");
    _(call, "ready") = _(ret, "ready");
    node printhex(_(call, "valid"));
    EgressInt(*val, _(_(call, "contents"), "arg0"));
    EgressFunc([val](bool valid) {
      if (valid)
        cout << "printhex> " << setw(8) << setfill('0') << hex << *val << dec
             << endl;
    }, _(call, "valid"));
  }

  template <typename T> using print_hex2_ret_t =
    flit<ag<STP("live"), T, ag<STP("rval"), bvec<0> > > >;
  template <typename T> using print_hex2_call_t =
    flit<ag<STP("live"), T, ag<STP("arg0"), ui<32>, ag<STP("arg1"), ui<32> > > > >;

  template <typename OPAQUE, typename G>
    void print_hex2(
      print_hex2_ret_t <OPAQUE> &ret,
      print_hex2_call_t<OPAQUE> &call,
      G &globals
    )
  {
    using namespace std;
    unsigned *val1 = new unsigned(), *val2 = new unsigned();
    
    _(ret, "valid") = _(call, "valid");
    _(call, "ready") = _(ret, "ready");
    node printhex(_(call, "valid"));
    EgressInt(*val1, _(_(call, "contents"), "arg0"));
    EgressInt(*val2, _(_(call, "contents"), "arg1"));
    EgressFunc([val1, val2](bool valid) {
      if (valid)
        cout << "printhex> " << setw(8) << setfill('0') << hex << *val1 << " " << setw(8) << setfill('0') << *val2 << dec << endl;
    }, _(call, "valid"));
  }
}
