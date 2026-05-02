cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

# SFML
FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.1.0
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(SFML)

# Box2D
FetchContent_Declare(box2d
        GIT_REPOSITORY https://github.com/erincatto/box2d.git
        GIT_TAG v2.4.1
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(box2d)

# Lua
FetchContent_Declare(
        lua
        URL https://www.lua.org/ftp/lua-5.4.8.tar.gz
)
FetchContent_MakeAvailable(lua)

file(GLOB LUA_SRC
        ${lua_SOURCE_DIR}/src/*.c
)

list(REMOVE_ITEM LUA_SRC
        ${lua_SOURCE_DIR}/src/lua.c
        ${lua_SOURCE_DIR}/src/luac.c
)

add_library(lua_static STATIC ${LUA_SRC})

target_include_directories(lua_static PUBLIC
        ${lua_SOURCE_DIR}/src
)

# Platform-specific Lua definitions
if(WIN32)
    target_compile_definitions(lua_static PRIVATE LUA_USE_WINDOWS)
elseif(APPLE)
    target_compile_definitions(lua_static PRIVATE LUA_USE_MACOSX)
elseif(UNIX)
    target_compile_definitions(lua_static PRIVATE LUA_USE_LINUX)
    target_link_libraries(lua_static PUBLIC dl m)
endif()

add_executable(main src/main.cpp)

target_compile_features(main PRIVATE cxx_std_17)

target_include_directories(main PRIVATE lib)

target_link_libraries(main PRIVATE
        SFML::Graphics
        lua_static
        box2d
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)