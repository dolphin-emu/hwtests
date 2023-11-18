add_library(hwtests_common
  hwtests.cpp
  timebase.h
  timebase.s
)

target_link_libraries(hwtests_common PUBLIC fmt::fmt)
