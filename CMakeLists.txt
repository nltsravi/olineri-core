cmake_minimum_required(VERSION 3.16)
project(olineri_core VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Core include directories
set(OLINERI_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include_directories(${OLINERI_INCLUDE_DIRS})

# Core source files
set(OLINERI_CORE_SOURCES
    core/src/crypto.cpp
    core/src/manifest.cpp
    core/src/package_manager.cpp
    core/src/scoring_engine.cpp
    core/src/g2p.cpp
    core/src/runtime.cpp
    backends/mock/mock_engine.cpp
)

# Static library
add_library(olineri_core_static STATIC ${OLINERI_CORE_SOURCES})
target_include_directories(olineri_core_static PUBLIC ${OLINERI_INCLUDE_DIRS})

# Shared library
add_library(olineri_core SHARED ${OLINERI_CORE_SOURCES})
target_include_directories(olineri_core PUBLIC ${OLINERI_INCLUDE_DIRS})
target_compile_definitions(olineri_core PRIVATE OLINERI_BUILD_SHARED)

# Tests
option(BUILD_TESTS "Build automated unit and integration tests" ON)

if(BUILD_TESTS)
    enable_testing()

    macro(olineri_add_test test_name test_source)
        add_executable(${test_name} ${test_source})
        target_link_libraries(${test_name} PRIVATE olineri_core_static)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endmacro()

    olineri_add_test(test_crypto core/tests/test_crypto.cpp)
    olineri_add_test(test_manifest core/tests/test_manifest.cpp)
    olineri_add_test(test_package_manager core/tests/test_package_manager.cpp)
    olineri_add_test(test_scoring core/tests/test_scoring.cpp)
    olineri_add_test(test_runtime core/tests/test_runtime.cpp)
    olineri_add_test(test_tamil_assessment core/tests/test_tamil_assessment.cpp)
    olineri_add_test(test_multi_language core/tests/test_multi_language.cpp)

    # Benchmarks
    add_executable(benchmark_engine benchmarks/benchmark_engine.cpp)
    target_link_libraries(benchmark_engine PRIVATE olineri_core_static)
endif()
