# build the Coco/R executable

cmake_minimum_required(VERSION 3.1)

project(coror)
  
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
       
add_executable( cocor src/Action.cpp
                      src/Action.h
                      src/BitArray.cpp
                      src/BitArray.h
                      src/CharClass.cpp
                      src/CharClass.h
                      src/CharSet.cpp
                      src/CharSet.h
                      src/Coco.cpp
                      src/Comment.cpp
                      src/Comment.h
                      src/DFA.cpp
                      src/DFA.h
                      src/Generator.cpp
                      src/Generator.h
                      src/Graph.h
                      src/HashTable.cpp
                      src/HashTable.h
                      src/Melted.cpp
                      src/Melted.h
                      src/Node.cpp
                      src/Node.h
                      src/NodeSymbolKind.h
                      src/Parser.cpp
                      src/Parser.h
                      src/ParserGen.cpp
                      src/ParserGen.h
                      src/Position.cpp
                      src/Position.h
                      src/Scanner.cpp
                      src/Scanner.h
                      src/Sets.h
                      src/SortedList.cpp
                      src/SortedList.h
                      src/State.cpp
                      src/State.h
                      src/StringBuilder.cpp
                      src/StringBuilder.h
                      src/Symbol.cpp
                      src/Symbol.h
                      src/Tab.cpp
                      src/Tab.h
                      src/Target.cpp
                      src/Target.h )
