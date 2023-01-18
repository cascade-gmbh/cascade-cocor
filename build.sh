#!/bin/bash

# set current directory to script source directory:
cd $(dirname $0)

# ensure environment:
if [[ -z "${CMAKE_BUILD_TYPE}" ]]; then
  CMAKE_BUILD_TYPE="Debug"
fi

echo ------------------------------------------------------- build-cocor

cmake .  
cmake --build . --config $CMAKE_BUILD_TYPE

if test -f "Debug/cocor.exe"; then
  cp ./Debug/cocor.exe ./cocor.exe

elif test -f "Release/cocor.exe"; then
  cp ./Release/cocor.exe ./cocor.exe

fi

echo COCOR TEST-CALL WITHOUT PARAMETERS:
./cocor

