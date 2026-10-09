cmake_minimum_required(VERSION 3.21)
project(TUNRUN VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
include(FetchContent)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TUNRUN_ENABLE_TESTS "Build TUNRUN tests" ON)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Immutable upstream raylib 5.5 commit, not a floating branch.
FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG c1ab645ca298a2801097931d1079b10ff7eb9df8
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(tunrun src/main.cpp)
target_include_directories(tunrun PRIVATE src)
target_link_libraries(tunrun PRIVATE raylib)

if(MSVC)
  target_compile_options(tunrun PRIVATE /W4 /permissive- /sdl)
  target_link_options(tunrun PRIVATE /DYNAMICBASE /NXCOMPAT /guard:cf)
  target_compile_definitions(tunrun PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
else()
  target_compile_options(tunrun PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(TUNRUN_ENABLE_TESTS)
  add_executable(tunrun_input_tests tests/input_state_tests.cpp)
  target_include_directories(tunrun_input_tests PRIVATE src)
  if(MSVC)
    target_compile_options(tunrun_input_tests PRIVATE /W4 /permissive- /sdl)
    target_link_options(tunrun_input_tests PRIVATE /DYNAMICBASE /NXCOMPAT /guard:cf)
  else()
    target_compile_options(tunrun_input_tests PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME input_state_tests COMMAND tunrun_input_tests)
endif()
