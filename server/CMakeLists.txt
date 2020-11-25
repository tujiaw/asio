CMAKE_MINIMUM_REQUIRED(VERSION 2.8) 
  
FILE(GLOB SOURCE_FILES *.cpp)
    
INCLUDE_DIRECTORIES(./
                    ../
                    ../asio
                    ../asio/internal
                    ../asio/proto
                    ${topDir}/linux/include)
                    
LINK_DIRECTORIES(${topDir}/linux/lib)
             
ADD_EXECUTABLE(asioserver ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(asioserver 
                      pthread
                      asio
                      boost_system
                      boost_date_time
                      boost_regex
                      boost_thread
                      glog
                      gflags
                      protobuf
					  z)
                      
