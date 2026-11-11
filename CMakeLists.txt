cmake_minimum_required(VERSION 3.25)

project(VigineFormat C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(VIGINEFORMAT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# The tests move to VigineUnitTest (task 18242); until then they stay off because googletest left external/.
option(VIGINEFORMAT_BUILD_TESTS "Build VigineFormat tests" OFF)
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

set(VIGINEFORMAT_GENERATED_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/generated)
# VigineFormat is a shared library: it exports only its own interfaces,
# factories and value types (marked VIGINEFORMAT_API); everything else,
# including the bundled third-party code, stays hidden inside.
add_library(VigineFormat SHARED ${VIGINEFORMAT_SOURCES} ${VIGINEFORMAT_HEADERS})
add_library(VigineFormat::VigineFormat ALIAS VigineFormat)

include(GenerateExportHeader)
generate_export_header(VigineFormat
    BASE_NAME VIGINEFORMAT
    EXPORT_MACRO_NAME VIGINEFORMAT_API
    EXPORT_FILE_NAME ${VIGINEFORMAT_GENERATED_INCLUDE}/vigine/format/vigineformat_export.h)

# Visibility, stated for every system rather than left to the defaults:
#  - hidden by default for C and C++ (a symbol is exported only when marked VIGINEFORMAT_API);
#  - inline member functions hidden as well;
#  - on Windows nothing is exported unless dllexport-marked (no export-all).
set_target_properties(VigineFormat PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    WINDOWS_EXPORT_ALL_SYMBOLS OFF
    POSITION_INDEPENDENT_CODE ON)
# A private STL member in an exported class is intended: the data is not part of the ABI contract.
if(MSVC)
    target_compile_options(VigineFormat PUBLIC /wd4251)
endif()

target_include_directories(VigineFormat
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PUBLIC $<BUILD_INTERFACE:${VIGINEFORMAT_GENERATED_INCLUDE}>
    # The third-party libraries stay implementation details of the .cpp
    # files; consumers never see them.
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/json/single_include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/tomlplusplus/include)

# --- rapidyaml -------------------------------------------------------------
# The sources include <ryml/rapidyaml.hpp>, the single-header amalgamation. It
# is generated from the rapidyaml fork's own sources (its tools/amalgamate.py,
# every option stated) rather than compiled as a library: the amalgamation is
# byte-identical to the release header the code was written against, so the
# behaviour, including RYML_SINGLE_HDR_DEFINE_NOW in one translation unit,
# does not change.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(VIGINEFORMAT_RYML_HEADER ${VIGINEFORMAT_GENERATED_INCLUDE}/ryml/rapidyaml.hpp)
add_custom_command(
    OUTPUT ${VIGINEFORMAT_RYML_HEADER}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${VIGINEFORMAT_GENERATED_INCLUDE}/ryml
    COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/external/rapidyaml/tools/amalgamate.py
            ${VIGINEFORMAT_RYML_HEADER} --c4core --fastfloat --stl -e tree
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/external/rapidyaml/tools/amalgamate.py
    VERBATIM)
add_custom_target(vigineformat_ryml_header DEPENDS ${VIGINEFORMAT_RYML_HEADER})
add_dependencies(VigineFormat vigineformat_ryml_header)

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
    set_target_properties(vigineformat_pdfio PROPERTIES C_STANDARD 11 POSITION_INDEPENDENT_CODE ON)
    set_target_properties(vigineformat_pdfio PROPERTIES C_VISIBILITY_PRESET hidden)
    add_dependencies(VigineFormat vigineformat_pdfio)
    target_link_libraries(VigineFormat PRIVATE ZLIB::ZLIB)
    target_include_directories(VigineFormat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/pdfio)
    # pdfio is compiled in and none of its symbols may leave the library:
    #  - macOS: -load_hidden pulls the archive in with every symbol private;
    #  - Linux: --exclude-libs,ALL keeps the archive's symbols out of the dynamic table;
    #  - Windows: a DLL exports only dllexport-marked symbols, so the archive links in as is.
    if(APPLE)
        target_link_options(VigineFormat PRIVATE "LINKER:-load_hidden,$<TARGET_FILE:vigineformat_pdfio>")
    elseif(UNIX)
        target_link_libraries(VigineFormat PRIVATE vigineformat_pdfio)
        target_link_options(VigineFormat PRIVATE "LINKER:--exclude-libs,ALL")
    else()
        target_link_libraries(VigineFormat PRIVATE vigineformat_pdfio)
    endif()
    target_compile_definitions(VigineFormat PRIVATE VIGINEFORMAT_HAS_PDFIO=1)
endif()

# Linux: besides hiding the archive, a version script exports only the vigine
# symbols, so the standard library's weak template instantiations stay private
# too. macOS (-load_hidden above) and Windows (dllexport only) need no list.
if(UNIX AND NOT APPLE)
    target_link_options(VigineFormat PRIVATE "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/cmake/vigineformat.map")
    set_property(TARGET VigineFormat APPEND PROPERTY LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/vigineformat.map)
endif()

# --- tests ---------------------------------------------------------------
# The tests move to VigineUnitTest (task 18242), which brings them back.
if(VIGINEFORMAT_BUILD_TESTS)
    message(FATAL_ERROR "VigineFormat holds no test framework; its tests come from VigineUnitTest")
endif()
