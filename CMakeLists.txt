cmake_minimum_required(VERSION 3.10)
project(WeaponDataBase C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Bundled SQLite amalgamation; single-threaded, no extensions -> no pthread/dl
add_library(sqlite3 STATIC sqlite3.c)
target_compile_definitions(sqlite3 PUBLIC SQLITE_THREADSAFE=0 SQLITE_OMIT_LOAD_EXTENSION)

add_executable(weapondb main.c db.c menu.c tables.c crud.c platform.c)
target_link_libraries(weapondb PRIVATE sqlite3)

if(MSVC)
    target_compile_options(weapondb PRIVATE /W4)
    target_compile_definitions(weapondb PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(weapondb PRIVATE -Wall -Wextra -pedantic)
endif()
