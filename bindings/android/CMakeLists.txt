cmake_minimum_required(VERSION 3.22.1)
project(vaani_jni LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VAANI_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

include_directories(
    ${VAANI_ROOT}/core/include
    ${VAANI_ROOT}
)

add_library(vaani_jni SHARED
    src/main/cpp/vaani_jni.cpp
    ${VAANI_ROOT}/core/src/crypto.cpp
    ${VAANI_ROOT}/core/src/manifest.cpp
    ${VAANI_ROOT}/core/src/package_manager.cpp
    ${VAANI_ROOT}/core/src/scoring_engine.cpp
    ${VAANI_ROOT}/core/src/g2p.cpp
    ${VAANI_ROOT}/core/src/runtime.cpp
    ${VAANI_ROOT}/backends/mock/mock_engine.cpp
)

find_library(log-lib log)
target_link_libraries(vaani_jni ${log-lib})
