#!/bin/bash

# set current directory to script source directory:
cd $(dirname $0)

echo ------------------------------------------------------- build-cocor

# invoke cmake
echo CMAKE
cmake .  
cmake --build . --config $CMAKE_BUILD_TYPE

# copy msvc build result
if test -f "Debug/cocor.exe"; then
  echo --> copy windows DEBUG executable
  cp ./Debug/cocor.exe ./cocor.exe
elif test -f "Release/cocor.exe"; then
  echo --> copy windows RELEASE executable
  cp ./Release/cocor.exe ./cocor.exe
fi

# simple test
echo COCOR TEST-CALL WITHOUT PARAMETERS:
./cocor


