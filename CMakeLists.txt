SET (CMAKE_C_COMPILER "gcc")
SET (CMAKE_CXX_COMPILER "g++")
PROJECT (Cavalia)
SET (CMAKE_CXX_FLAGS "-std=c++11 -DOCC -DNUMA" CACHE INTERNAL "compiler options" FORCE)
SET (CMAKE_CXX_FLAGS_DEBUG "-O0 -g" CACHE INTERNAL "compiler options" FORCE)
SET (CMAKE_CXX_FLAGS_RELEASE "-O3" CACHE INTERNAL "compiler options" FORCE)
IF (NOT CMAKE_BUILD_TYPE)
	SET (CMAKE_BUILD_TYPE "Release")
ENDIF ()
MESSAGE (STATUS "Build type: ${CMAKE_BUILD_TYPE}")
CMAKE_MINIMUM_REQUIRED (VERSION 2.8)

INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/../protobuf/include)
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/../boost/include)
LINK_DIRECTORIES (${PROJECT_SOURCE_DIR}/../protobuf/lib)
LINK_DIRECTORIES (${PROJECT_SOURCE_DIR}/../boost/lib)

INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/Common)
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/BenchmarkFramework)
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/StorageEngine)
LINK_DIRECTORIES (${PROJECT_SOURCE_DIR}/build/StorageEngine)
ADD_SUBDIRECTORY (StorageEngine)
ADD_SUBDIRECTORY (TpccBenchmark)
