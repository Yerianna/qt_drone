CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)
project(qt_drone)


set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Make it automatically compiles Qt project
set(CMAKE_AUTOMOC ON)

# Find the OpenCV library
find_package(OpenCV)

# Find the ros and related libraries
find_package(catkin REQUIRED COMPONENTS  ardrone_autonomy    cv_bridge  image_transport  roscpp  sensor_msgs  std_msgs)

# Find the QtWidgets library
set(CMAKE_PREFIX_PATH /home/arianna/qt5/qt/5.4/gcc_64)

find_package(Qt5Widgets)
find_package(Qt5Core)
find_package(Qt5Gui)





catkin_package()

#include_directories( ${catkin_INCLUDE_DIRS})

#include_directories( ${OpenCV_INCLUDE_DIRS})

# The UI Frame
add_executable(UI src/qt/mainwindow.h src/qt/main.cpp src/qt/mainwindow.cpp src/qt/mainwindow.ui)
target_link_libraries(UI Qt5::Widgets Qt5::Gui Qt5::Core ${OpenCV_LIBRARIES})

# The controller
add_executable(controller src/controller.cpp)
target_link_libraries(controller ${catkin_LIBRARIES})



