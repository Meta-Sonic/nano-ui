cmake_minimum_required(VERSION 3.22.0)

include(FetchContent)


# Add nano cmake.
# execute_process(COMMAND wget -O - https://nano-cmake.onrender.com/nano.cmake OUTPUT_VARIABLE NANO_CMAKE ERROR_QUIET)
# cmake_language(EVAL CODE ${NANO_CMAKE})

project(nano-ui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NANO_UI_BUILD_TESTS "Build tests" ON)
option(NANO_UI_BUILD_EXAMPLES "Build examples" ON)

# Fetch nano-common.
if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../nano-common")
    set(FETCHCONTENT_SOURCE_DIR_NANO_COMMON "${CMAKE_CURRENT_SOURCE_DIR}/../nano-common")
endif()

FetchContent_Declare(nano_common GIT_REPOSITORY "https://github.com/Meta-Sonic/nano-common.git")
FetchContent_MakeAvailable(nano_common)

# nano-ui lib.
set(NAME ui)
set(MODULE_NAME nano-ui)

# nano_add_module(geometry DEV_MODE)
nano_add_module(graphics DEV_MODE)
nano_add_module(objc DEV_MODE)

set(NANO_UI_SRC_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/nano")

file(GLOB_RECURSE NANO_UI_SOURCE_FILES
    "${NANO_UI_SRC_DIRECTORY}/*.h"
    "${NANO_UI_SRC_DIRECTORY}/*.cpp")

add_library(${MODULE_NAME} STATIC ${NANO_UI_SOURCE_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NANO_UI_SOURCE_FILES})
target_include_directories(${MODULE_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${MODULE_NAME} PUBLIC nano::graphics nano::objc)

add_library(nano::${NAME} ALIAS ${MODULE_NAME})

set_target_properties(${MODULE_NAME} PROPERTIES XCODE_GENERATE_SCHEME OFF)

if (APPLE) 
    target_link_libraries(${MODULE_NAME} PUBLIC
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreText"
        "-framework ImageIO"
        "-framework CoreServices"
        "-framework AppKit"
    )
else()
    # target_link_libraries(${PROJECT_NAME} PUBLIC
    #     Gdiplus.lib
    #     d2d1
    # )
    
endif()

if (NANO_UI_DEV)
    set(NANO_UI_BUILD_TESTS ON)
    # nano_clang_format(${MODULE_NAME} ${NANO_UI_SOURCE_FILES})
endif()

if (NANO_UI_BUILD_TESTS)
    nano_add_module(test)

    file(GLOB_RECURSE TEST_SOURCE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h")

    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/tests" FILES ${TEST_SOURCE_FILES})

    set(TEST_NAME nano-${NAME}-tests)
    add_executable(${TEST_NAME} ${TEST_SOURCE_FILES})
    target_include_directories(${TEST_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    target_link_libraries(${TEST_NAME} PUBLIC nano::test ${MODULE_NAME})

    set(CLANG_OPTIONS -Weverything -Wno-c++98-compat)
    set(MSVC_OPTIONS /W4)

    target_compile_options(${TEST_NAME} PUBLIC
        "$<$<CXX_COMPILER_ID:Clang,AppleClang>:${CLANG_OPTIONS}>"
        "$<$<CXX_COMPILER_ID:MSVC>:${MSVC_OPTIONS}>")

    # set_target_properties(${TEST_NAME} PROPERTIES CXX_STANDARD 20)
endif()

# file(GLOB_RECURSE NANO_UI_SOURCE_FILES
#     "${NANO_GRAPHICS_SRC_DIRECTORY}/*.h"
#     "${NANO_GRAPHICS_SRC_DIRECTORY}/*.cpp")

# set(NANO_UI_INCLUDE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
# set(NANO_UI_SOURCE_DIRECTORY "${NANO_UI_INCLUDE_DIRECTORY}/nano")

# file(GLOB_RECURSE NANO_UI_SOURCE_FILES "${NANO_UI_SOURCE_DIRECTORY}/*.cpp" "${NANO_UI_SOURCE_DIRECTORY}/*.h")

# add_library(${PROJECT_NAME} STATIC ${NANO_UI_SOURCE_FILES})
# source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NANO_UI_SOURCE_FILES})
# target_include_directories(${PROJECT_NAME} PUBLIC ${NANO_UI_INCLUDE_DIRECTORY})

# if (APPLE) 
#     target_link_libraries(${PROJECT_NAME} PUBLIC
#         "-framework CoreFoundation"
#         "-framework CoreGraphics"
#         "-framework CoreText"
#         "-framework AppKit"
#         "-framework WebKit"
#     )
# else()
#     target_link_libraries(${PROJECT_NAME} PUBLIC
#         Gdiplus.lib
#         d2d1
#     )
    
# endif()

# # Tests.
# if (NANO_UI_BUILD_TESTS)
#     add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)  
# endif()

# Example.
if (NANO_UI_BUILD_EXAMPLES)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)  
endif()
