#!/bin/bash

# set current directory to script source directory:
cd $(dirname $0)

echo ------------------------------------------------------- build-cocor

# invoke cmake
echo CMAKE
cmake .  
cmake --build . --config $CMAKE_BUILD_TYPE

# copy build result
echo copy executable to $CMAKE_OUTPUT_HOME
mkdir -p $CMAKE_OUTPUT_HOME
if test -f "cocor"; then
    cp ./cocor $CMAKE_OUTPUT_HOME/
elif test -f "Debug/cocor.exe"; then
    echo -- windows DEBUG build
    cp ./Debug/cocor.exe $CMAKE_OUTPUT_HOME/cocor.exe
elif test -f "Release/cocor.exe"; then
    echo -- windows RELEASE build
    cp ./Release/cocor.exe $CMAKE_OUTPUT_HOME/cocor.exe
fi

# simple test
echo COCOR TEST-CALL WITHOUT PARAMETERS:
cd $CMAKE_OUTPUT_HOME
./cocor


