
add_library(munit "${CMAKE_CURRENT_SOURCE_DIR}/munit/munit.c")
target_include_directories(munit PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/munit")

add_executable(chiaki-unit
		main.c
		http.c
		rpcrypt.c
		gkcrypt.c
		takion.c
		seqnum.c
		keystate.c
		reorderqueue.c
		fec.c
		test_log.c
		test_log.h
		bitstream.c
		regist.c
		cloudcatalog_merge.c
		cloudsession_kamaji.c
		pschord.c
		auth_classifier_test.cpp)

find_package(spdlog REQUIRED)
set_target_properties(chiaki-unit PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(chiaki-unit chiaki-lib munit spdlog::spdlog)
if(TARGET PkgConfig::json-c)
	target_link_libraries(chiaki-unit PkgConfig::json-c)
endif()

add_test(unit chiaki-unit)
