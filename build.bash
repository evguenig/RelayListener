#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Usage: $0 <build_path>"
  exit 1
fi

if [ -z "$1" ]; then
  echo "Parameter cannot be empty."
  echo "Usage: $0 <parameter>"
  exit 1
fi

cmake=/usr/bin/cmake
# flags="-DCMAKE_C_COMPILER=/usr/bin/gcc  -DCMAKE_CXX_COMPILER=/usr/bin/g++"


build_path=$1

pushd .

if [ ! -d "$build_path" ]; then
  mkdir -p "$build_path"
  echo "Created folder $build_path for build"
  curr_dir=`pwd`
  cd $build_path
  $cmake $curr_dir 
else
  echo "Using folder $build_path for build"
  cd $build_path
  build_dir=`pwd`
  $cmake --build $build_dir --target clean
fi

build_dir=`pwd`
$cmake --build $build_dir --target all -j 6

popd

