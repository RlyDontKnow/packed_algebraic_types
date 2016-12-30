cmake_minimum_required(VERSION 3.3)

set(CMAKE_CONFIGURATION_TYPES Release CACHE TYPE INTERNAL FORCE)
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

project(packed_algebraic_types CXX)

set(TESTSCRIPT "" CACHE PATH "Optionally execute tests through a script (e.g. to test on a target, run valgrind on them, etc.)")

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /wd4127 /wd4510 /wd4512 /wd4610")
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
else(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unknown-pragmas -Wno-missing-field-initializers")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif(MSVC)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# setup for general testing
enable_testing()
add_custom_target(BUILD_TESTS)
add_custom_target(TEST COMMAND ${CMAKE_CTEST_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR} -C $<CONFIGURATION> --verbose)
add_dependencies(TEST BUILD_TESTS)
set_property(TARGET TEST PROPERTY FOLDER "Tests")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.cmake" "${CMAKE_BINARY_DIR}/CTestCustom.cmake")
set(gtest_force_shared_crt ON CACHE BOOL "")
add_subdirectory(contrib/gtest)

# macro to create a regular test case
macro(make_simple_test MODULE FUNC FILE)
  set(TESTTARGET ${MODULE}_${FUNC})
  set(TESTSRC ${CMAKE_CURRENT_BINARY_DIR}/${TESTTARGET}.cpp)
  set(TESTFILE ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.cpp)

  configure_file(${CMAKE_SOURCE_DIR}/test.cpp.in ${TESTSRC})
  add_executable(${TESTTARGET} EXCLUDE_FROM_ALL ${TESTSRC})
  target_link_libraries(${TESTTARGET} gtest)
  target_include_directories(${TESTTARGET} PRIVATE ${CMAKE_SOURCE_DIR}/contrib/gtest/include)
  add_dependencies(BUILD_TESTS ${TESTTARGET})
  set_property(TARGET ${TESTTARGET} PROPERTY FOLDER "Tests/${MODULE}")

  if("${TESTSCRIPT}" STREQUAL "")
    add_test(NAME ${TESTTARGET} COMMAND $<TARGET_FILE:${TESTTARGET}>)
  else()
    add_test(NAME ${TESTTARGET} COMMAND ${TESTSCRIPT} $<TARGET_FILE:${TESTTARGET}>)
  endif()

  if(${ARGC} GREATER 3)
    set(EXPECT_FAILURE ON)
  else()
    set(EXPECT_FAILURE OFF)
  endif()
  if(EXPECT_FAILURE)
    set_tests_properties(${TESTTARGET} PROPERTIES WILL_FAIL TRUE)
  endif()
endmacro()

# macro to create a compile test (expect a specific compile failure)
macro(make_static_assert_test MODULE FUNC FILE MSG)
  set(TESTSRC ${CMAKE_CURRENT_BINARY_DIR}/${FILE}_${FUNC}.cpp)
  set(TESTFILE ${CMAKE_CURRENT_SOURCE_DIR}/${FILE}.cpp)
  set(TESTTARGET ${MODULE}_${FUNC})

  configure_file(${CMAKE_SOURCE_DIR}/test.cpp.in ${TESTSRC})
  add_executable(${TESTTARGET} EXCLUDE_FROM_ALL ${TESTSRC})
  set_target_properties(${TESTTARGET} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD 1)
  target_link_libraries(${TESTTARGET} gtest)
  target_include_directories(${TESTTARGET} PRIVATE ${CMAKE_SOURCE_DIR}/contrib/gtest/include)
  add_test(NAME ${TESTTARGET} COMMAND ${CMAKE_COMMAND} --build . --target ${TESTTARGET} --config $<CONFIGURATION> WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  set_property(TARGET ${TESTTARGET} PROPERTY FOLDER "Tests/${MODULE}")
  set_tests_properties(${TESTTARGET} PROPERTIES PASS_REGULAR_EXPRESSION "${MSG}")
endmacro()

# grab our tests
add_subdirectory(test)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/doxyfile)
  add_custom_target(documentation
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
  )
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc-output/ DESTINATION ${CMAKE_INSTALL_PREFIX}/doc OPTIONAL)
endif()
