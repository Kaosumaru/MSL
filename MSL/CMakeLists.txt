cmake_minimum_required (VERSION 2.8)


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

##folders
add_folder(include)
add_folder(src)
add_library(MSL ${SRCS})

##dependencies

find_package(Boost REQUIRED)
include_directories( ${Boost_INCLUDE_DIRS} )
include_directories( ${PROJECT_SOURCE_DIR} )
##find_package(allegro REQUIRED)
##message(STATUS "Using SDL, include: ${SDL_INCLUDE_DIR}")