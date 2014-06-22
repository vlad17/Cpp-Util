cmake_minimum_required(VERSION 2.6)
project(Cpp_Util)

# Compilation options

set(CMAKE_CXX_COMPILER g++)
set(GENERAL_FLAGS "-Wall -Wextra -std=c++11 -pthread -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -O0 ${GENERAL_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG ${GENERAL_FLAGS}")

# Get build

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE debug)
endif (NOT CMAKE_BUILD_TYPE)

#executable generation

unset(ALL_LIBS CACHE)
function(ADD_LIB)
  get_filename_component(FDIR ${CMAKE_CURRENT_LIST_DIR} NAME)
  add_library(${FDIR} ${ARGN})
  set(ALL_LIBS ${ALL_LIBS} ${FDIR} CACHE INTERNAL "")
endfunction()

# All subdirectories
set(ALL_DIRS pthread_wrappers fibheap caches queues utilities)

# Adds a test with dependencies on all directories
function(ADD_TEST_ALL name)
  add_executable(${name} "${name}.cpp")
  target_link_libraries(${name} ${ALL_LIBS})
endfunction()

# Uses additional arguments to add link libraries on top of the local one
function(ADD_TEST name)
  get_filename_component(FDIR ${CMAKE_CURRENT_LIST_DIR} NAME)
  # check if local library exists
  list(FIND ALL_LIBS "${FDIR}" result)
  if (result EQUAL -1)
    unset(FDIR)
  endif (result EQUAL -1)
  add_executable(${name} "${name}.cpp")
  target_link_libraries(${name} ${FDIR} ${ARGN})
endfunction()

# TODO figure out cmake testing enable_testing()

### configure paths

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
string(TOLOWER ${CMAKE_BUILD_TYPE} BUILDLOWER)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT}/${BUILDLOWER})

include_directories(src)

foreach(libdir ${ALL_DIRS})
  add_subdirectory(src/${libdir})
endforeach(libdir)


