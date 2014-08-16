set(COMMON_CODE
                test.cpp)

add_hwtest(cputest fctiwz     fctiwz.cpp)
add_hwtest(cputest frsp       frsp.cpp)
add_hwtest(cputest reciprocal reciprocal.cpp)
add_hwtest(cputest mtspr      mtspr.cpp)
add_hwtest(cputest srawix     srawix.cpp)
