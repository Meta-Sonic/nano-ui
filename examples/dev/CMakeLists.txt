cmake_minimum_required(VERSION 3.22.0)

project(nano-dev)

file(GLOB_RECURSE EXAMPLE_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"  "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EXAMPLE_SOURCE_FILES})

add_executable(${PROJECT_NAME} ${EXAMPLE_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC nano::ui)

# set(CLANG_OPTIONS -Weverything -Wno-c++98-compat)
# set(MSVC_OPTIONS /W4)

target_compile_options(${PROJECT_NAME} PUBLIC
    "$<$<CXX_COMPILER_ID:Clang,AppleClang>:${CLANG_OPTIONS}>"
    "$<$<CXX_COMPILER_ID:MSVC>:${MSVC_OPTIONS}>"
)

set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_GENERATE_SCHEME YES)