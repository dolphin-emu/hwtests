add_executable(gxtest source/cgx.cpp source/gxtest_util.cpp source/main.cpp source/Test.cpp)
target_link_libraries(gxtest wiiuse bte fat ogc m)
