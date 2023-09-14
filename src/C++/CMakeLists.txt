
cmake_minimum_required(VERSION 3.0)
project(cpptracker)
set(cpptracker PROJECT_NAME)
set(CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _CRT_SECURE_NO_WARNINGS /MP /utf-8")
# add_definitions(-DUNICODE -D_UNICODE)
set(Boost_lib_DIR "D:\\lib\\boost_1_83_0\\lib")
set(Boost_Include_DIR "D:\\lib\\boost_1_83_0\\include\\boost-1_83")


# add header path
include_directories(
    ${Boost_Include_DIR}
)
# set common source
file(GLOB SRC
   ./*.cpp
)
file (GLOB PLATFORM_SRC

)

# generate binary
add_executable(${PROJECT_NAME}  ${SRC})


link_libraries(${PROJECT_NAME} ${Boost_lib_DIR})
