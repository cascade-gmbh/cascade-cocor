#!/bin/bash

# set current directory to script source directory:
cd $(dirname $0)

echo ------------------------------------------------------- build-cocor

# invoke cmake
echo CMAKE:
cmake .  
cmake --build . --config $CMAKE_BUILD_TYPE

# copy build result
echo EXECUTABLE:
if test -f "cocor"; then
  cp ./cocor ./$TARGET_SPEC-cocor
elif test -f "Debug/cocor.exe"; then
  cp ./Debug/cocor.exe ./$TARGET_SPEC-cocor.exe
elif test -f "Release/cocor.exe"; then
  cp ./Release/cocor.exe ./$TARGET_SPEC-cocor.exe
fi
ls $TARGET_SPEC-*

# simple test
echo COCOR TEST-CALL WITHOUT PARAMETERS:
./$TARGET_SPEC-cocor


