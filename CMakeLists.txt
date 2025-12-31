cmake_minimum_required(VERSION 3.10)
project(MyBigProject VERSION 1.0)

# 1. Use C++17 or C++20
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Tell the compiler to look in the 'include' folder for headers
include_directories(include)

# 3. Collect ALL .cpp files from the 'src' folder automatically
file(GLOB SOURCES "src/*.cpp")

# 4. Name your executable (e.g., "my_app")
add_executable(ALO-COMPILER ${SOURCES})