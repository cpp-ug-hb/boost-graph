cmake_minimum_required(VERSION 3.5)
project(bgl-demo)

set(CMAKE_CXX_STANDARD 14)

find_package(Boost REQUIRED COMPONENTS graph)

add_executable(scheduler
	scheduler/scheduler.cpp
)

target_include_directories(scheduler PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(scheduler PRIVATE ${Boost_LIBRARIES})


add_executable(visualizer
  visualizer/visualizer.cpp
)

target_include_directories(visualizer PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(visualizer PRIVATE ${Boost_LIBRARIES})
