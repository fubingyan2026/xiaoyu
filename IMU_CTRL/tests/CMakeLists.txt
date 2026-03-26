cmake_minimum_required(VERSION 3.22)

project(IMU_CTRL_TESTS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Find common modules to test
# For embedded code, we need to mock hardware dependencies or use pure algorithm code
# Since this is an STM32 project, we test pure algorithm modules only

# Include paths for common modules
# Note: common/ is at parent directory of IMU_CTRL (i.e., xiaoyu/)
# So from tests/ directory, we need ../../common
set(COMMON_ROOT ${CMAKE_SOURCE_DIR}/../../common)
set(TEST_INCLUDE_DIRS
    ${COMMON_ROOT}/algorithm
    ${COMMON_ROOT}/controller
    ${COMMON_ROOT}/protocol
)

include_directories(${TEST_INCLUDE_DIRS})

# Collect test source files
# Note: Only test pure algorithm modules that don't depend on hardware
set(TEST_SOURCES
    pid_test.cpp
)

# Create the test executable
add_executable(run_tests ${TEST_SOURCES})

# Link GoogleTest and the common modules
target_link_libraries(run_tests
    gtest_main
    pthread
)

# Link common modules (copy the algorithm/controller sources for testing)
# For simplicity, we'll compile them directly
# Note: common/ is at parent directory of IMU_CTRL (i.e., xiaoyu/)
# So from tests/ directory, we need ../../common
# Only test pure algorithm modules that don't depend on hardware (HAL)
set(COMMON_PATH ${CMAKE_SOURCE_DIR}/../../common)
target_sources(run_tests PRIVATE
    ${COMMON_PATH}/controller/pid.c
    # gimbal_pid.c depends on bsp_delay.h (hardware abstraction), skip for unit test
    ${COMMON_PATH}/algorithm/maths.c
    ${COMMON_PATH}/algorithm/filter.c
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(run_tests)
