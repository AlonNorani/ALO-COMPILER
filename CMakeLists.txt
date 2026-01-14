cmake_minimum_required(VERSION 3.10)
project(MyBigProject VERSION 1.0)

# 1. Standard C++ Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. Collect ALL .cpp files automatically 
# ADDITION: CONFIGURE_DEPENDS tells CMake to re-scan for new files 
# every time you build, so you don't have to manually "Reconfigure".
file(GLOB SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# 3. Create the executable
add_executable(ALO-COMPILER ${SOURCES})

# 4. Modern Header Inclusion
# We attach the 'include' folder directly to the ALO-COMPILER target.
# This ensures that any file in ALO-COMPILER can find headers in /include.
target_include_directories(ALO-COMPILER PRIVATE include)