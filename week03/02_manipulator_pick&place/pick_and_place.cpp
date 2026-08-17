#include <rclcpp/rclcpp.hpp>

#include <moveit/move_group_interface/move_group_interface.hpp>
#include <moveit/planning_scene_interface/planning_scene_interface.hpp>

#include <moveit_msgs/msg/collision_object.hpp>
#include <moveit_msgs/msg/robot_trajectory.hpp>

#include <shape_msgs/msg/solid_primitive.hpp>
#include <geometry_msgs/msg/pose.hpp>

#include <chrono>
#include <thread>
#include <vector>
#include <string>

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>(
      "pick_and_place",
      rclcpp::NodeOptions()
          .automatically_declare_parameters_from_overrides(true));

  // ROS callback 처리
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node);

  std::thread executor_thread([&executor]()
  {
    executor.spin();
  });


  // ============================================================
  // MoveIt Interfaces
  // ============================================================

  moveit::planning_interface::MoveGroupInterface arm(
      node,
      "panda_arm");

  moveit::planning_interface::MoveGroupInterface hand(
      node,
      "hand");

  moveit::planning_interface::PlanningSceneInterface
      planning_scene_interface;


  arm.setPlanningTime(20.0);
  arm.setNumPlanningAttempts(20);

  arm.setMaxVelocityScalingFactor(0.2);
  arm.setMaxAccelerationScalingFactor(0.2);


  // ============================================================
  // STEP 0 : 기존 Object 제거
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 0 : Removing old object");

  planning_scene_interface.removeCollisionObjects(
      {"pick_box"});

  std::this_thread::sleep_for(
      std::chrono::seconds(1));


  // ============================================================
  // STEP 1 : Box 생성
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 1 : Adding pick_box");


  moveit_msgs::msg::CollisionObject object;

  object.header.frame_id = "panda_link0";
  object.id = "pick_box";


  shape_msgs::msg::SolidPrimitive primitive;

  primitive.type =
      shape_msgs::msg::SolidPrimitive::BOX;

  primitive.dimensions = {
      0.05,
      0.05,
      0.10
  };


  geometry_msgs::msg::Pose object_pose;

  object_pose.orientation.w = 1.0;

  object_pose.position.x = 0.45;
  object_pose.position.y = 0.00;
  object_pose.position.z = 0.05;


  object.primitives.push_back(primitive);

  object.primitive_poses.push_back(object_pose);

  object.operation =
      moveit_msgs::msg::CollisionObject::ADD;


  planning_scene_interface.applyCollisionObject(object);


  std::this_thread::sleep_for(
      std::chrono::seconds(1));


  // ============================================================
  // STEP 2 : Gripper Open
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 2 : Opening gripper");


  hand.setNamedTarget("open");


  if (!static_cast<bool>(hand.move()))
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "Failed to open gripper");

    executor.cancel();
    executor_thread.join();
    rclcpp::shutdown();

    return 1;
  }


  // ============================================================
  // STEP 3 : Pre-Grasp
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 3 : Moving to pre-grasp");


  geometry_msgs::msg::Pose target_pose;


  // Gripper 아래 방향
  target_pose.orientation.x = 1.0;
  target_pose.orientation.y = 0.0;
  target_pose.orientation.z = 0.0;
  target_pose.orientation.w = 0.0;


  target_pose.position.x = 0.45;
  target_pose.position.y = 0.00;
  target_pose.position.z = 0.30;


  arm.setPoseTarget(target_pose);


  if (!static_cast<bool>(arm.move()))
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "Pre-grasp failed");

    executor.cancel();
    executor_thread.join();
    rclcpp::shutdown();

    return 1;
  }


  arm.clearPoseTargets();


  // ============================================================
  // STEP 4 : Cartesian Approach
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 4 : Approaching object");


  geometry_msgs::msg::Pose approach_pose =
      arm.getCurrentPose().pose;


  // 아래로 8 cm
  approach_pose.position.z -= 0.08;


  std::vector<geometry_msgs::msg::Pose> waypoints;

  waypoints.push_back(approach_pose);


  moveit_msgs::msg::RobotTrajectory trajectory;


  double fraction =
      arm.computeCartesianPath(
          waypoints,
          0.01,
          trajectory,
          true);


  RCLCPP_INFO(
      node->get_logger(),
      "Approach path: %.1f%%",
      fraction * 100.0);


  if (fraction < 0.95)
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "Approach failed");

    executor.cancel();
    executor_thread.join();
    rclcpp::shutdown();

    return 1;
  }


  if (!static_cast<bool>(
      arm.execute(trajectory)))
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "Approach execution failed");

    executor.cancel();
    executor_thread.join();
    rclcpp::shutdown();

    return 1;
  }


  // ============================================================
  // STEP 5 : Gripper Close
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 5 : Closing gripper");


  hand.setNamedTarget("close");


  if (!static_cast<bool>(hand.move()))
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "Close failed");

    executor.cancel();
    executor_thread.join();
    rclcpp::shutdown();

    return 1;
  }


  // ============================================================
  // STEP 6 : Attach
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 6 : Attaching object");


  std::vector<std::string> touch_links = {
      "panda_hand",
      "panda_leftfinger",
      "panda_rightfinger"
  };


  if (!arm.attachObject(
      "pick_box",
      "panda_hand",
      touch_links))
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "Attach failed");

    executor.cancel();
    executor_thread.join();
    rclcpp::shutdown();

    return 1;
  }


  std::this_thread::sleep_for(
      std::chrono::seconds(1));


  // ============================================================
  // STEP 7 : Lift
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 7 : Lifting object");


  geometry_msgs::msg::Pose lift_pose =
      arm.getCurrentPose().pose;


  lift_pose.position.z += 0.12;


  waypoints.clear();

  waypoints.push_back(lift_pose);


  moveit_msgs::msg::RobotTrajectory lift_trajectory;


  fraction =
      arm.computeCartesianPath(
          waypoints,
          0.01,
          lift_trajectory,
          true);


  if (fraction < 0.95)
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "Lift path failed");

    executor.cancel();
    executor_thread.join();
    rclcpp::shutdown();

    return 1;
  }


  arm.execute(lift_trajectory);


  // ============================================================
  // STEP 8 : Place 위치로 이동
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 8 : Moving to place position");


  geometry_msgs::msg::Pose place_pose =
      arm.getCurrentPose().pose;


  // 물체를 옆으로 이동
  place_pose.position.x = 0.35;
  place_pose.position.y = 0.30;
  place_pose.position.z = 0.30;


  arm.setPoseTarget(place_pose);


  if (!static_cast<bool>(arm.move()))
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "Move to place failed");

    executor.cancel();
    executor_thread.join();
    rclcpp::shutdown();

    return 1;
  }


  arm.clearPoseTargets();


  // ============================================================
  // STEP 9 : Lower
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 9 : Lowering object");


  geometry_msgs::msg::Pose lower_pose =
      arm.getCurrentPose().pose;


  lower_pose.position.z -= 0.10;


  waypoints.clear();

  waypoints.push_back(lower_pose);


  moveit_msgs::msg::RobotTrajectory lower_trajectory;


  fraction =
      arm.computeCartesianPath(
          waypoints,
          0.01,
          lower_trajectory,
          true);


  if (fraction < 0.95)
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "Lower path failed");

    executor.cancel();
    executor_thread.join();
    rclcpp::shutdown();

    return 1;
  }


  arm.execute(lower_trajectory);


  // ============================================================
  // STEP 10 : Gripper Open
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 10 : Opening gripper");


  hand.setNamedTarget("open");

  hand.move();


  // ============================================================
  // STEP 11 : Detach
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 11 : Detaching object");


  if (!arm.detachObject("pick_box"))
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "Detach failed");

    executor.cancel();
    executor_thread.join();
    rclcpp::shutdown();

    return 1;
  }


  std::this_thread::sleep_for(
      std::chrono::seconds(1));


  // ============================================================
  // STEP 12 : Retreat
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 12 : Retreat");


  geometry_msgs::msg::Pose retreat_pose =
      arm.getCurrentPose().pose;


  retreat_pose.position.z += 0.10;


  waypoints.clear();

  waypoints.push_back(retreat_pose);


  moveit_msgs::msg::RobotTrajectory retreat_trajectory;


  fraction =
      arm.computeCartesianPath(
          waypoints,
          0.01,
          retreat_trajectory,
          true);


  if (fraction >= 0.95)
  {
    arm.execute(retreat_trajectory);
  }


  RCLCPP_INFO(
      node->get_logger(),
      "====================================");

  RCLCPP_INFO(
      node->get_logger(),
      "PICK AND PLACE COMPLETE");

  RCLCPP_INFO(
      node->get_logger(),
      "====================================");


  executor.cancel();

  if (executor_thread.joinable())
  {
    executor_thread.join();
  }


  rclcpp::shutdown();

  return 0;
}
