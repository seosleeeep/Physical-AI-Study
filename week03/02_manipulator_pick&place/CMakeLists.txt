cmake_minimum_required(VERSION 3.8)

project(panda_pick_place)


# ============================================================
# Compiler Options
# ============================================================

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
  )
endif()


# ============================================================
# Dependencies
# ============================================================

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

find_package(moveit_ros_planning_interface REQUIRED)
find_package(moveit_msgs REQUIRED)

find_package(shape_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)


# ============================================================
# add_object
# ============================================================

add_executable(
  add_object
  src/add_object.cpp
)

ament_target_dependencies(
  add_object
  rclcpp
  moveit_ros_planning_interface
  moveit_msgs
  shape_msgs
  geometry_msgs
)


# ============================================================
# gripper_control
# ============================================================

add_executable(
  gripper_control
  src/gripper_control.cpp
)

ament_target_dependencies(
  gripper_control
  rclcpp
  moveit_ros_planning_interface
)


# ============================================================
# attach_object
# ============================================================

add_executable(
  attach_object
  src/attach_object.cpp
)

ament_target_dependencies(
  attach_object
  rclcpp
  moveit_ros_planning_interface
)


# ============================================================
# detach_object
# ============================================================

add_executable(
  detach_object
  src/detach_object.cpp
)

ament_target_dependencies(
  detach_object
  rclcpp
  moveit_ros_planning_interface
)


# ============================================================
# pick_object
# ============================================================

add_executable(
  pick_object
  src/pick_object.cpp
)

ament_target_dependencies(
  pick_object
  rclcpp
  moveit_ros_planning_interface
  moveit_msgs
  geometry_msgs
)


# ============================================================
# pick_and_place
# ============================================================

add_executable(
  pick_and_place
  src/pick_and_place.cpp
)

ament_target_dependencies(
  pick_and_place
  rclcpp
  moveit_ros_planning_interface
  moveit_msgs
  shape_msgs
  geometry_msgs
)


# ============================================================
# Install
# ============================================================

install(
  TARGETS
  add_object
  gripper_control
  attach_object
  detach_object
  pick_object
  pick_and_place

  DESTINATION lib/${PROJECT_NAME}
)


# ============================================================
# Package
# ============================================================

ament_package()
