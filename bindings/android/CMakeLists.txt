cmake_minimum_required(VERSION 3.22.1)
project(olineri_jni LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OLINERI_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

include_directories(
    ${OLINERI_ROOT}/core/include
    ${OLINERI_ROOT}
)

add_library(olineri_jni SHARED
    src/main/cpp/olineri_jni.cpp
    ${OLINERI_ROOT}/core/src/crypto.cpp
    ${OLINERI_ROOT}/core/src/manifest.cpp
    ${OLINERI_ROOT}/core/src/package_manager.cpp
    ${OLINERI_ROOT}/core/src/scoring_engine.cpp
    ${OLINERI_ROOT}/core/src/g2p.cpp
    ${OLINERI_ROOT}/core/src/runtime.cpp
    ${OLINERI_ROOT}/backends/mock/mock_engine.cpp
)

find_library(log-lib log)
target_link_libraries(olineri_jni ${log-lib})
