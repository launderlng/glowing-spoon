cmake_minimum_required(VERSION 3.20)
project(xzx LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build the GUI app. WIN32 = no console window on launch.
add_executable(xzx WIN32 src/main.cpp)

# Force the output file to be exactly xzx.exe for every compiler/config.
set_target_properties(xzx PROPERTIES OUTPUT_NAME "xzx")

target_compile_definitions(xzx PRIVATE UNICODE _UNICODE _WIN32_WINNT=0x0A00)
target_link_libraries(xzx PRIVATE comctl32 shell32)

if(MSVC)
  target_compile_options(xzx PRIVATE /W4 /permissive-)
endif()
