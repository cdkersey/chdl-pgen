#!/bin/bash
for x in *.chdl.cpp; do
  echo === $x ===
  cp $x ../chdl-runtime/cgen-out.incl
  pushd ../chdl-runtime
  make
  ./chdl-example > OUTPUT.$x
  popd
done
