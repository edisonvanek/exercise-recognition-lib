cmake_minimum_required(VERSION 3.20)
project(ExerciseRecognitionLib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${OpenCV_INCLUDE_DIRS})

# Main library
add_library(exercise_recognition
    src/exercise_recognizer.cpp
    src/squat_counter.cpp
    src/pushup_counter.cpp
    src/pullup_counter.cpp
)

target_link_libraries(exercise_recognition PUBLIC ${OpenCV_LIBS})

# Test executable for Windows camera feed
if(WIN32)
    add_executable(camera_test tests/windows_camera_test.cpp)
    target_link_libraries(camera_test PRIVATE exercise_recognition ${OpenCV_LIBS})
    
    # Windows-specific libraries
    target_link_libraries(camera_test PRIVATE user32 gdi32)
endif()

# Generic camera test (cross-platform)
add_executable(camera_test_generic tests/camera_test.cpp)
target_link_libraries(camera_test_generic PRIVATE exercise_recognition ${OpenCV_LIBS})
