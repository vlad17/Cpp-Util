# CMakeLists file for sync directory

ADD_LIB(
  countdown_latch.cpp
  rwlock.cpp
)

ADD_EXEC(ptw-test)
ADD_EXEC(cdl-test)

