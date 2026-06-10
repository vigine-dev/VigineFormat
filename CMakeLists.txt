cmake_minimum_required(VERSION 3.25)

project(VigineFormat C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(VIGINEFORMAT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

option(VIGINEFORMAT_BUILD_TESTS "Build VigineFormat tests" ON)
option(VIGINEFORMAT_WITH_PDF "Build the PDF backend (vendored pdfio + system zlib)" ON)

if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# --- facade library ------------------------------------------------------
# Identify-then-parse for many file formats: content-first identification
# (magic bytes + structural probes, the extension only as a hint) and parsers
# that emit NEUTRAL models -- no third-party type ever appears in a public
# header, so backends can be swapped without touching consumers.
file(GLOB VIGINEFORMAT_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB VIGINEFORMAT_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/vigine/format/*.h)
# zlib is the PDF backend's only system dependency. Probe it softly: hosts
# without zlib (a bare Windows runner) still build the library, with
# PdfImporter linking as an always-unsupported stub instead of dropping out
# of the binary and breaking consumers at link time.
if(VIGINEFORMAT_WITH_PDF)
    find_package(ZLIB)
    if(NOT ZLIB_FOUND)
        message(STATUS "VigineFormat: zlib not found -- PDF backend disabled, PdfImporter reports unsupported")
        set(VIGINEFORMAT_WITH_PDF OFF)
    endif()
endif()

add_library(VigineFormat STATIC ${VIGINEFORMAT_SOURCES} ${VIGINEFORMAT_HEADERS})
add_library(VigineFormat::VigineFormat ALIAS VigineFormat)

target_include_directories(VigineFormat
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    # The vendored single-header JSON parser stays an implementation detail of
    # the .cpp files; consumers never see it.
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/tomlplusplus/include)

# --- PDF backend ----------------------------------------------------------
# pdfio is a small plain-C library with no CMake of its own: its sources
# compile straight into a private static target. Needs only the system zlib.
if(VIGINEFORMAT_WITH_PDF)
    file(GLOB VIGINEFORMAT_PDFIO_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/external/pdfio/pdfio-*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/external/pdfio/ttf.c)
    add_library(vigineformat_pdfio STATIC ${VIGINEFORMAT_PDFIO_SOURCES})
    target_link_libraries(vigineformat_pdfio PRIVATE ZLIB::ZLIB)
    target_include_directories(vigineformat_pdfio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/pdfio)
    set_target_properties(vigineformat_pdfio PROPERTIES C_STANDARD 11)
    target_link_libraries(VigineFormat PRIVATE vigineformat_pdfio)
    target_compile_definitions(VigineFormat PRIVATE VIGINEFORMAT_HAS_PDFIO=1)
endif()

# --- tests ---------------------------------------------------------------
if(VIGINEFORMAT_BUILD_TESTS)
    set(gtest_force_shared_crt ON CACHE BOOL "Use shared CRT for GoogleTest" FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/googletest ${CMAKE_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)
    enable_testing()
    file(GLOB VIGINEFORMAT_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test/*_test.cpp)
    if(NOT VIGINEFORMAT_WITH_PDF)
        list(REMOVE_ITEM VIGINEFORMAT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test/pdf_test.cpp)
    endif()
    add_executable(vigineformat-test ${VIGINEFORMAT_TEST_SOURCES})
    target_link_libraries(vigineformat-test PRIVATE VigineFormat gtest gtest_main)
    set_target_properties(vigineformat-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    include(GoogleTest)
    gtest_discover_tests(vigineformat-test DISCOVERY_TIMEOUT 60 DISCOVERY_MODE PRE_TEST)
endif()
