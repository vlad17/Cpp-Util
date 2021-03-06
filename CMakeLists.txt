cmake_minimum_required(VERSION 2.6)
project(Cpp_Util)

# Assertions can be controlled with -DASSERTIONS
# 0 (or anything not 1 or 2) - on in debug but off in release.
# 1 - manually activated
# 2 - manually deactivated

if("${ASSERTIONS}" EQUAL 1)
  message("overwriting build setting and enabling assertions")
  add_definitions(-DUSE_UASSERT=1)
elseif("${ASSERTIONS}" EQUAL 2)
  message("overwriting build setting and disabling assertions")
  add_definitions(-DUSE_UASSERT=0)
endif()

# Boost

find_package(Boost
  1.53.0 # first release with lockfree headers
  # [REQUIRED] COMPONENTS
) # no components needed, just headers

if(${Boost_FOUND})
  include_directories(SYSTEM ${BOOST_INCLUDE_DIRS})
  add_definitions(-DHAVE_BOOST)
else()
endif()

# Compilation options

if(NOT ${CMAKE_CXX_COMPILER})
  set(CMAKE_CXX_COMPILER g++)
endif()
if("${CMAKE_CXX_COMPILER}" STREQUAL "g++")
  set(GENERAL_FLAGS "--enable-tls")
endif()
set(GENERAL_FLAGS "-Wall -Wextra -std=c++11 -pthread -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -O0 ${GENERAL_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG ${GENERAL_FLAGS}")
set(CTEST_OUTPUT_ON_FAIL TRUE)

# Get build

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE debug)
endif (NOT CMAKE_BUILD_TYPE)

# Get output location

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
string(TOLOWER ${CMAKE_BUILD_TYPE} BUILDLOWER)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT}/${BUILDLOWER})
set(OUTDIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTDIR})

#executable generation

# libraries are always implicitly dependent on 'util'
unset(ALL_LIBS CACHE)
function(ADD_LIB)
  get_filename_component(FDIR ${CMAKE_CURRENT_SOURCE_DIR} NAME)
  add_library(${FDIR} ${ARGN})
  set(ALL_LIBS ${ALL_LIBS} ${FDIR} CACHE INTERNAL "")
endfunction()

# All subdirectories
set(ALL_DIRS synchro fibheap caches queues util)

# Adds a test with dependencies on all directories
function(ADD_EXEC_ALL_DEPENDENCIES name)
  add_executable(${name} "${name}.cpp")
  target_link_libraries(${name} ${ALL_LIBS})
endfunction()

enable_testing()

# Uses additional arguments to add link libraries on top of the local one
# always add implicit 'util' dependency
function(ADD_EXEC name)
  get_filename_component(FDIR ${CMAKE_CURRENT_SOURCE_DIR} NAME)
  # check if local library exists
  list(FIND ALL_LIBS "${FDIR}" result)
  if (result EQUAL -1)
    unset(FDIR)
  endif (result EQUAL -1)
  add_executable("${name}.exe" "${name}.cpp")
  add_test(NAME ${name} COMMAND "${name}.exe" WORKING_DIRECTORY ${OUTDIR})
  target_link_libraries("${name}.exe" ${FDIR} util ${ARGN})
endfunction()

### configure subdirectories

include_directories(src)

foreach(libdir ${ALL_DIRS})
  add_subdirectory(src/${libdir})
endforeach(libdir)


