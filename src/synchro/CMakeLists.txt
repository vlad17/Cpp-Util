# CMakeLists file for sync directory

ADD_LIB(
  countdown_latch.cpp
  hazard.cpp
  rwlock.cpp
)

ADD_EXEC(ptw-test)
ADD_EXEC(hazard-test)
ADD_EXEC(cdl-test util)

