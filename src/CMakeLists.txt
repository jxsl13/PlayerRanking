cmake_minimum_required(VERSION 3.10)
project(rankingserver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADER_FILES
playerstats.h
rankingserver.h
)

set(SOURCE_FILES 
${HEADER_FILES}
main.cpp
rankingserver.cpp
playerstats.cpp
)


###
# includes
###
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cpp_redis/includes"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cpp_redis/tacopie/includes"
)

###
# libraries
###
link_directories(${DEPS_LIBRARIES})


add_subdirectory(3rdparty/cpp_redis)

target_include_directories(cpp_redis PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cpp_redis/includes"
    "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cpp_redis/tacopie/includes"
)


add_executable(rankingserver ${SOURCE_FILES})
target_link_libraries(rankingserver cpp_redis)



