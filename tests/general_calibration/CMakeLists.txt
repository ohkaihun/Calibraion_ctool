target_sources(calibmar_tests PRIVATE
	general_calibration_test.cpp
	stereo_calibration_test.cpp)

add_test(NAME general_calibration_test COMMAND calibmar_tests)
add_test(NAME stereo_calibration_test COMMAND calibmar_tests)