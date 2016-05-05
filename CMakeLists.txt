cmake_minimum_required (VERSION 2.6)

set (PROJECT RussianDependencyParser)
project (${PROJECT})
add_definitions (-g -Wall -pedantic -std=c++11)

include_directories(src)
include_directories(marisa)
include_directories(tinyxml2)

set(SOURCES_MARISA
	marisa/marisa/agent.cc
	marisa/marisa/grimoire/io/mapper.cc
	marisa/marisa/grimoire/io/reader.cc
	marisa/marisa/grimoire/io/writer.cc
	marisa/marisa/grimoire/trie/louds-trie.cc
	marisa/marisa/grimoire/trie/tail.cc
	marisa/marisa/grimoire/vector/bit-vector.cc
	marisa/marisa/keyset.cc
	marisa/marisa/trie.cc)

set(SOURCES_GENDICT
	src/gendict.cpp
	src/paradigm.h
	src/paradigm.cpp)

set(SOURCES_TINYXML2
	tinyxml2/tinyxml2.cpp)

set(SOURCES_MAIN
	src/model.cpp
	src/model.h
	src/main.cpp
	src/paradigm.h
	src/paradigm.cpp)

add_library(marisa STATIC ${SOURCES_MARISA})

add_executable (gendict ${SOURCES_GENDICT})
target_link_libraries(gendict marisa)

add_library(tinyxml2 STATIC ${SOURCES_TINYXML2})

add_executable (main ${SOURCES_MAIN})
target_link_libraries (main marisa tinyxml2)
