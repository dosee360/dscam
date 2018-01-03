cmake_minimum_required(VERSION 2.8)  
project(demo)    

find_package(OpenCV REQUIRED) 

FILE(GLOB src "*.cpp")
add_executable(demo ${src})
target_link_libraries(demo ${OpenCV_LIBS} -lv4l2 -lpthread)    
