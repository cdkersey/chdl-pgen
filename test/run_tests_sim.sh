#!/bin/bash
for x in *.sim.cpp; do
  echo === $x ===
    
  cp $x ../cpp-runtime/cgen-out.incl
  pushd ../cpp-runtime
  make clean; make
  ./cpp-example > OUTPUT.$x
  popd
done
