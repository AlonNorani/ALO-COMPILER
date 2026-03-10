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

# 5. Add src folder to include path (so CodeGenerator.h is found)
target_include_directories(ALO-COMPILER PRIVATE src)

# 6. Find LLVM using llvm-config
execute_process(COMMAND llvm-config --cxxflags OUTPUT_VARIABLE LLVM_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --ldflags OUTPUT_VARIABLE LLVM_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --libs core support irreader OUTPUT_VARIABLE LLVM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

# Add LLVM flags and includes
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LLVM_CXXFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
include_directories(${LLVM_INCLUDE_DIR})

# 7. Link LLVM libraries
target_link_libraries(ALO-COMPILER ${LLVM_LDFLAGS} ${LLVM_LIBS})