set(COMMON_CODE
                cgx.cpp
                gxtest_util.cpp
                Test.cpp)

add_hwtest(gxtest bitfield      bitfield.cpp)
add_hwtest(gxtest clipping      clipping.cpp)
add_hwtest(gxtest lighting      lighting.cpp)
add_hwtest(gxtest rasterization rasterization.cpp)
add_hwtest(gxtest tev           tev.cpp)
