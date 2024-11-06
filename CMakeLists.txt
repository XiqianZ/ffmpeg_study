# CMakeList.txt : CMake project for socket_study2, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("app_video_stream")
message(STATUS "Project: ${PROJECT_NAME}")



#==============================================================================
# Set Opencv directory
#==============================================================================
find_package(OpenCV 4.9 REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
message(STATUS "========: OpenCV version: ${OpenCV_VERSION}")
message(STATUS "========: OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")


#==============================================================================
# Set source files
#==============================================================================
file(GLOB_RECURSE SRC_SOURCES 
	src/*.hpp
	src/*.cpp
	src/*.h
)

file(GLOB_RECURSE SRC_SERVER
	src/server/*.hpp
	src/server/*.cpp
)

file (GLOB_RECURSE SRC_CODEC
	src/codec/*.hpp
	src/codec/*.cpp
)


#==============================================================================
# Set webscocketpp directory
#==============================================================================
#file(GLOB_RECURSE WEBSOCKETPP 
#	vendors/include/websocketpp/*.hpp
#)



#====================================
# Copy DLLs to executable folder
#====================================
file(COPY "${CMAKE_SOURCE_DIR}/vendors/ffmpeg/bin/" DESTINATION "${CMAKE_BINARY_DIR}")
message(STATUS "Need to copy: " ${CMAKE_SOURCE_DIR}/vendors/ffmpeg/bin)
message(STATUS "Server target directory: " ${CMAKE_BINARY_DIR}/bin)



#==============================================================================
# Windows
#==============================================================================
if(WIN32)
	message(STATUS "========: Windows")
	add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()



# Add source to this project's executable.
#add_executable (server "main_server.cpp" ${SRC_SERVER} ${SRC_CODEC})
#target_include_directories(server PRIVATE src/server src/codec vendors/include vendors/ffmpeg/include)
#target_link_directories(server PRIVATE vendors/lib vendors/ffmpeg/lib)
#target_link_libraries(server PRIVATE ${OpenCV_LIBS} avcodec avformat avutil avdevice swscale)



#add_executable (client "main_client.cpp" ${SRC_CODEC})
#target_include_directories(client PRIVATE src/client src/codec vendors/include vendors/ffmpeg/include)
#target_link_directories(client PRIVATE ${CMAKE_SOURCE_DIR}/vendors/lib ${CMAKE_SOURCE_DIR}/vendors/ffmpeg/lib)
#target_link_libraries(client PUBLIC ${OpenCV_LIBS} avcodec avformat avutil avdevice swscale)



#add_executable(codec "main_codec.cpp")
#target_include_directories(codec PRIVATE vendors/include vendors/ffmpeg/include)
#target_link_directories(codec PRIVATE vendors/lib vendors/ffmpeg/lib)
#target_link_libraries(codec PRIVATE ${OpenCV_LIBS} avcodec avformat avutil avdevice swscale)


add_executable(codec264 "main_264codec.cpp")
target_include_directories(codec264 PRIVATE vendors/include vendors/ffmpeg/include)
target_link_directories(codec264 PRIVATE vendors/lib vendors/ffmpeg/lib)
target_link_libraries(codec264 PRIVATE ${OpenCV_LIBS} avcodec avformat avutil avdevice swscale)





if (CMAKE_VERSION VERSION_GREATER 3.12)
  #set_property(TARGET server PROPERTY CXX_STANDARD 20)
  #set_property(TARGET client PROPERTY CXX_STANDARD 20)
endif()
# TODO: Add tests and install targets if needed.

