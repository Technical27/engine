cmake_minimum_required(VERSION 3.16)
project(engine)

set (CMAKE_CXX_STANDARD 17)

set (COMMON_FLAGS "-std=c++17 -Wall -Wextra -pedantic")

set (CMAKE_CXX_FLAGS_DEBUG "${COMMON_FLAGS} -Og -g3 -DUSE_VALIDATION_LAYERS")
set (CMAKE_CXX_FLAGS_RELEASE "${COMMON_FLAGS} -O2 -s")

find_package(SDL2 REQUIRED FATAL_ERROR)
find_package(Vulkan REQUIRED FATAL_ERROR)
find_package(glm REQUIRED FATAL_ERROR)

add_executable(
  main
  src/main.cpp
  src/vulkan.cpp
  src/sdl.cpp
)

add_custom_command(
  TARGET main
  POST_BUILD
  COMMAND ${CMAKE_SOURCE_DIR}/compile-shaders.sh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Compiling Shaders"
)

include_directories("${CMAKE_SOURCE_DIR}/include")
target_link_libraries(main SDL2 Vulkan::Vulkan glm ${CMAKE_DL_LIBS})
