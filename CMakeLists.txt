cmake_minimum_required(VERSION 3.20)
project(ExerciseRecognitionLib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch OpenCV from GitHub
FetchContent_Declare(
    opencv
    GIT_REPOSITORY https://github.com/opencv/opencv.git
    GIT_TAG 4.8.1
    GIT_SHALLOW TRUE
)

# Set OpenCV options
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_PERF_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WITH_CUDA OFF CACHE BOOL "" FORCE)
set(WITH_VTK OFF CACHE BOOL "" FORCE)
set(WITH_WEBP OFF CACHE BOOL "" FORCE)

message(STATUS "Fetching OpenCV... This may take a few minutes on first build")
FetchContent_MakeAvailable(opencv)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Main library
add_library(exercise_recognition
    src/exercise_recognizer.cpp
    src/squat_counter.cpp
    src/pushup_counter.cpp
    src/pullup_counter.cpp
)

target_include_directories(exercise_recognition PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${opencv_INCLUDE_DIRS}
)

target_link_libraries(exercise_recognition PUBLIC opencv_core opencv_imgproc opencv_videoio)

# Test executable for Windows camera feed
if(WIN32)
    add_executable(camera_test tests/windows_camera_test.cpp)
    target_include_directories(camera_test PRIVATE ${opencv_INCLUDE_DIRS})
    target_link_libraries(camera_test PRIVATE exercise_recognition opencv_core opencv_imgproc opencv_videoio)
    
    # Windows-specific libraries
    target_link_libraries(camera_test PRIVATE user32 gdi32)
endif()

# Generic camera test (cross-platform)
add_executable(camera_test_generic tests/camera_test.cpp)
target_include_directories(camera_test_generic PRIVATE ${opencv_INCLUDE_DIRS})
target_link_libraries(camera_test_generic PRIVATE exercise_recognition opencv_core opencv_imgproc opencv_videoio)
