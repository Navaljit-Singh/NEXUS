cmake_minimum_required(VERSION 3.25)
project(NEXUS VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type defaults
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags (GCC/Clang)
add_compile_options(
  -Wall -Wextra -Wpedantic -Werror
  -Wno-unused-parameter
  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:Release>:-march=native>
  $<$<CONFIG:Debug>:-O0>
  $<$<CONFIG:Debug>:-g3>
  $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)
add_link_options($<$<CONFIG:Debug>:-fsanitize=address,undefined>)

# Include directories
include_directories(include)

# Source files grouped by subsystem
set(NEXUS_CORE_SOURCES
    src/core/logger.cpp)
set(NEXUS_MARKET_DATA_SOURCES
    src/market_data/upstox_feed.cpp)
set(NEXUS_ORDER_BOOK_SOURCES
    src/order_book/order_book.cpp
    src/order_book/bar_aggregator.cpp)
set(NEXUS_RISK_SOURCES
    src/risk/risk_manager.cpp)
set(NEXUS_OMS_SOURCES
    src/oms/order_manager.cpp)
set(NEXUS_STRATEGY_SOURCES
    src/strategy/strategy_base.cpp
    src/strategy/sample_strategy.cpp)

# NEXUS static library
add_library(nexus_lib STATIC
    ${NEXUS_CORE_SOURCES}
    ${NEXUS_MARKET_DATA_SOURCES}
    ${NEXUS_ORDER_BOOK_SOURCES}
    ${NEXUS_RISK_SOURCES}
    ${NEXUS_OMS_SOURCES}
    ${NEXUS_STRATEGY_SOURCES})
target_include_directories(nexus_lib PUBLIC include)
target_link_libraries(nexus_lib PRIVATE pthread)

# Main engine executable
add_executable(nexus_engine apps/nexus_engine/main.cpp)
target_link_libraries(nexus_engine PRIVATE nexus_lib)

# Test executables (using CTest)
enable_testing()

set(TEST_TARGETS
    test_spsc_queue
    test_order_book
    test_risk_manager
    test_indicators
)

foreach(test_name ${TEST_TARGETS})
    add_executable(${test_name} tests/${test_name}.cpp)
    target_include_directories(${test_name} PRIVATE include)
    target_link_libraries(${test_name} PRIVATE nexus_lib pthread)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
