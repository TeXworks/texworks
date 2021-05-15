cmake_minimum_required(VERSION 3.0)

project(Lua)

file(GLOB LIBSOURCES src/*.c)
file(GLOB LUASOURCES src/lua.c)
file(GLOB LUACSOURCES src/luac.c)

list(REMOVE_ITEM LIBSOURCES ${LUASOURCES} ${LUACSOURCES})

add_library(lua ${LIBSOURCES})
set_target_properties(lua PROPERTIES PUBLIC_HEADER "src/lua.h;src/luaconf.h;src/lualib.h;src/lauxlib.h;src/lua.hpp")

install(TARGETS lua)
