cmake_minimum_required(VERSION 2.8)
project(ut)

set(SOURCES source/framework.cpp
            source/server_test.cpp)

set(COMMON_LIBS cppkit)
set(WINDOWS_LIBS cppkit)

include(common.cmake NO_POLICY_SCOPE)
include(../../devel_artifacts/build/base_ut.cmake)
