cmake_minimum_required(VERSION 2.8)

project(test-pyuv)

set (CMAKE_BUILD_TYPE Debug)

include_directories(/opt/libuv/include)
link_directories(/opt/libuv/lib)

#add_executable(test-uv-async test-uv-async)
#target_link_libraries(test-uv-async uv)

include_directories(/opt/libuv/include)
link_directories(/opt/libuv/lib)
add_executable(VoiceSlave httppost.cpp)
add_executable(test_uv_work test_libuv_queue_work.cpp)
#add_executable(VoiceSlave main.c)
#target_link_libraries(VoiceSlave uv curl)
target_link_libraries(VoiceSlave uv curl)
target_link_libraries(test_uv_work uv curl)