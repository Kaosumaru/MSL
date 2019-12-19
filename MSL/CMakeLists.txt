cmake_minimum_required (VERSION 2.8)
set(CMAKE_CXX_STANDARD 17)

macro (add_sources)
    file (RELATIVE_PATH _relPath "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
    foreach (_src ${ARGN})
        if (_relPath)
            list (APPEND SRCS "${_relPath}/${_src}")
        else()
            list (APPEND SRCS "${_src}")
        endif()
    endforeach()
    if (_relPath)
        # propagate SRCS to parent directory
        set (SRCS ${SRCS} PARENT_SCOPE)
    endif()
endmacro()

macro (add_folder name)
add_subdirectory(${name})
string(REPLACE "/" "\\" filtername ${name})
source_group(${filtername} FILES ${files})
endmacro()

project (MSL)


if (MSVC)
add_definitions( "/W3 /D_CRT_SECURE_NO_WARNINGS /wd4018 /wd4005 /wd4996 /nologo" )
endif(MSVC)
if (EMSCRIPTEN)
  add_definitions("-std=c++14")
endif (EMSCRIPTEN)
##folders
add_folder(msl)
add_folder(src)

if(BUILD_TESTS)
    add_executable(MSL tests/main.cpp ${SRCS})
else(BUILD_TESTS)
    add_library(MSL ${SRCS})
endif(BUILD_TESTS)
##dependencies

include_directories( ${PROJECT_SOURCE_DIR} )
include_directories( ${PROJECT_SOURCE_DIR}/../deps/PEGTL )
##find_package(allegro REQUIRED)
##message(STATUS "Using SDL, include: ${SDL_INCLUDE_DIR}")
