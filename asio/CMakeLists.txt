CMAKE_MINIMUM_REQUIRED(VERSION 2.8)  

ADD_DEFINITIONS(-DTHREADED)
FILE(GLOB SOURCE_FILES
     *.cpp
     internal/*.cpp
     proto/*.cc
	 tool/*.cpp)
  
INCLUDE_DIRECTORIES(. ${topDir}/linux/include)
 
ADD_LIBRARY(asio ${SOURCE_FILES}) 

