cmake_minimum_required(VERSION 3.16)
project(vaani_core VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Core include directories
set(VAANI_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include_directories(${VAANI_INCLUDE_DIRS})

# Core source files
set(VAANI_CORE_SOURCES
    core/src/crypto.cpp
    core/src/manifest.cpp
    core/src/package_manager.cpp
    core/src/scoring_engine.cpp
    core/src/g2p.cpp
    core/src/runtime.cpp
    backends/mock/mock_engine.cpp
)

# Static library
add_library(vaani_core_static STATIC ${VAANI_CORE_SOURCES})
target_include_directories(vaani_core_static PUBLIC ${VAANI_INCLUDE_DIRS})

# Shared library
add_library(vaani_core SHARED ${VAANI_CORE_SOURCES})
target_include_directories(vaani_core PUBLIC ${VAANI_INCLUDE_DIRS})
target_compile_definitions(vaani_core PRIVATE VAANI_BUILD_SHARED)

# Tests
option(BUILD_TESTS "Build automated unit and integration tests" ON)

if(BUILD_TESTS)
    enable_testing()

    macro(vaani_add_test test_name test_source)
        add_executable(${test_name} ${test_source})
        target_link_libraries(${test_name} PRIVATE vaani_core_static)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endmacro()

    vaani_add_test(test_crypto core/tests/test_crypto.cpp)
    vaani_add_test(test_manifest core/tests/test_manifest.cpp)
    vaani_add_test(test_package_manager core/tests/test_package_manager.cpp)
    vaani_add_test(test_scoring core/tests/test_scoring.cpp)
    vaani_add_test(test_runtime core/tests/test_runtime.cpp)
    vaani_add_test(test_tamil_assessment core/tests/test_tamil_assessment.cpp)
    vaani_add_test(test_multi_language core/tests/test_multi_language.cpp)
endif()
