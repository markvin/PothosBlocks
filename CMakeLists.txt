########################################################################
# PothosBlocks: general purpose processing blocks for Pothos.
# Builds against an installed Pothos core (find_package(Pothos)).
# The modules use only the public API of the core. Their third party
# libraries (json, xsimd, Asio) come from find_package: from the system,
# or from a package manager that gives a toolchain file.
########################################################################
cmake_minimum_required(VERSION 3.25)
project(PothosBlocks CXX)

option(BLOCKS_BUILD_TESTS "Register the CTest tests of the blocks" ${PROJECT_IS_TOP_LEVEL})
option(BLOCKS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors (GCC and Clang)" OFF)

#the core defines the variable POTHOS_ROOT (its install prefix), and the
#runtime environment often sets POTHOS_ROOT too. find_package must not use
#them as search prefixes (CMP0144): it could find another core than
#CMAKE_PREFIX_PATH gives. NO_PACKAGE_ROOT_PATH keeps them out.
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

find_package(Pothos CONFIG REQUIRED NO_PACKAGE_ROOT_PATH)
include(PothosConfigSIMD) #pothos_multiarch, PothosGenerateSIMDSources

#the core loads modules from its own prefix (POTHOS_ROOT): install there
#when the user does not give CMAKE_INSTALL_PREFIX
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${POTHOS_ROOT}" CACHE PATH "Install prefix (default: the prefix of the Pothos core)" FORCE)
    message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX} (the prefix of the Pothos core)")
endif()

########################################################################
# Third party libraries
########################################################################
set(blocks_missing_dependencies)

find_package(nlohmann_json 3.11 CONFIG)
if(NOT nlohmann_json_FOUND)
    list(APPEND blocks_missing_dependencies "nlohmann_json (>= 3.11)")
endif()

#Asio: a package configuration when there is one, else the headers
find_package(asio 1.38 CONFIG QUIET)
if(asio_FOUND)
    message(STATUS "Asio: config package (${asio_DIR})")
else()
    list(PREPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
    find_package(asio 1.38 MODULE)
    if(asio_FOUND)
        message(STATUS "Asio: headers (Findasio.cmake, ${asio_INCLUDE_DIR})")
    else()
        list(APPEND blocks_missing_dependencies "asio (>= 1.38)")
    endif()
endif()

find_package(xsimd 14.2 CONFIG)
if(NOT xsimd_FOUND)
    list(APPEND blocks_missing_dependencies "xsimd (>= 14.2)")
endif()

if(blocks_missing_dependencies)
    list(JOIN blocks_missing_dependencies ", " blocks_missing_text)
    message(FATAL_ERROR
        "PothosBlocks: missing build dependencies: ${blocks_missing_text}.\n"
        "Install the system packages (for example: asio, nlohmann-json, xsimd), "
        "or install them with a package manager and give its toolchain file "
        "(-DCMAKE_TOOLCHAIN_FILE=...) or CMAKE_PREFIX_PATH. Refer to README.md.")
endif()

add_library(blocks_json INTERFACE)
target_link_libraries(blocks_json INTERFACE nlohmann_json::nlohmann_json)

add_library(blocks_asio INTERFACE)
target_link_libraries(blocks_asio INTERFACE asio::asio)
target_compile_definitions(blocks_asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
if (WIN32)
    target_compile_definitions(blocks_asio INTERFACE _WIN32_WINNT=0x0A00)
    target_link_libraries(blocks_asio INTERFACE ws2_32 mswsock)
endif ()

add_library(blocks_xsimd INTERFACE)
target_link_libraries(blocks_xsimd INTERFACE xsimd)

########################################################################
# Modules
########################################################################
#pothos_blocks_module(TARGET <name> SOURCES <sources...>
#    [LIBRARIES <libraries...>] [TESTS <self test names...>])
#builds and installs a blocks module, TESTS are names under /blocks/tests
#run one by one by the blocks.selftests test
function(pothos_blocks_module)
    cmake_parse_arguments(BLOCKS "" "TARGET" "SOURCES;LIBRARIES;TESTS" ${ARGN})
    POTHOS_MODULE_UTIL(
        TARGET ${BLOCKS_TARGET}
        SOURCES ${BLOCKS_SOURCES}
        LIBRARIES blocks_json ${BLOCKS_LIBRARIES}
        DESTINATION blocks
        ENABLE_DOCS)
    set_target_properties(${BLOCKS_TARGET} PROPERTIES CXX_EXTENSIONS OFF)
    #the repository root for common/
    target_include_directories(${BLOCKS_TARGET} PRIVATE ${PROJECT_SOURCE_DIR})
    if(BLOCKS_WARNINGS_AS_ERRORS AND NOT MSVC)
        target_compile_options(${BLOCKS_TARGET} PRIVATE -Werror)
    endif()
    foreach(test IN LISTS BLOCKS_TESTS)
        set_property(GLOBAL APPEND PROPERTY POTHOS_BLOCKS_TESTS /blocks/tests/${test})
    endforeach()
endfunction()

add_subdirectory(event)
add_subdirectory(file)
add_subdirectory(network)
add_subdirectory(packet)
add_subdirectory(serialize)
add_subdirectory(stream)
add_subdirectory(testers)

########################################################################
# Tests: the modules are staged into a prefix inside the build tree and
# loaded with POTHOS_PLUGIN_PATH by the PothosUtil of the installed core
########################################################################
if(BLOCKS_BUILD_TESTS)
    enable_testing()
    set(BLOCKS_STAGE_DIR ${PROJECT_BINARY_DIR}/stage)
    set(BLOCKS_SELFTEST_TIMEOUT 900 CACHE STRING "Timeout in seconds for the self-tests")

    #the installed core (POTHOS_ROOT from PothosConfig.cmake): PothosUtil is in
    #<root>/bin (POTHOS_UTIL_EXE is a wrapper script in this build directory)

    add_test(NAME blocks.stage
        COMMAND ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR} --prefix ${BLOCKS_STAGE_DIR} --config $<CONFIG>)
    set_tests_properties(blocks.stage PROPERTIES FIXTURES_SETUP blocks_stage)

    get_property(blocks_tests GLOBAL PROPERTY POTHOS_BLOCKS_TESTS)
    add_test(NAME blocks.selftests
        COMMAND ${CMAKE_COMMAND}
            -DPREFIX=${POTHOS_ROOT}
            -DTEST_HOME_DIR=${PROJECT_BINARY_DIR}/test-home
            -DBINDIR=bin
            -DLIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DPLUGIN_PATH=${BLOCKS_STAGE_DIR}/${CMAKE_INSTALL_LIBDIR}/Pothos/modules${POTHOS_ABI_VERSION}
            "-DTEST_PATHS=${blocks_tests}"
            -P ${PROJECT_SOURCE_DIR}/cmake/PothosRunSelfTests.cmake)
    set_tests_properties(blocks.selftests PROPERTIES
        FIXTURES_REQUIRED blocks_stage
        TIMEOUT ${BLOCKS_SELFTEST_TIMEOUT})
endif()
