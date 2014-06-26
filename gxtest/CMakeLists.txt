set(COMMON_CODE
               source/cgx.cpp
               source/gxtest_util.cpp
               source/Test.cpp)

add_hwtest(gxtest bitfield      source/bitfield.cpp)
add_hwtest(gxtest clipping      source/clipping.cpp)
add_hwtest(gxtest lighting      source/lighting.cpp)
add_hwtest(gxtest rasterization source/rasterization.cpp)
add_hwtest(gxtest tev           source/tev.cpp)
