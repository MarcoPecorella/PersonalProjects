cmake_minimum_required(VERSION 3.15)
project(TrackLookUpService VERSION 1.0 LANGUAGES CXX)

# --------------------
# Build type and sanitizers
# --------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)

# --------------------
# Compiler settings
# --------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warning flags
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Debug flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
endif()

# Sanitizer flags
if(ENABLE_SANITIZERS)
    if(NOT MSVC)
        add_compile_options(-fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# --------------------
# Source files
# --------------------
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/model")
file(GLOB SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
        "${SRC_DIR}/*.cpp"
)

# --------------------
# Executable target
# --------------------
add_executable(TrackLookUpService ${SOURCES})
target_include_directories(TrackLookUpService
        PRIVATE ${SRC_DIR}
)
target_compile_features(TrackLookUpService PRIVATE cxx_std_20)

target_link_libraries(TrackLookUpService PRIVATE)

# --------------------
# Installation rules
# --------------------
install(TARGETS TrackLookUpService
        RUNTIME DESTINATION bin
)
install(DIRECTORY ${SRC_DIR}/
        DESTINATION include/model
        FILES_MATCHING PATTERN "*.h"
)
