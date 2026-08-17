#include <rclcpp/rclcpp.hpp>

#include <moveit/move_group_interface/move_group_interface.hpp>

#include <moveit_msgs/msg/robot_trajectory.hpp>

#include <geometry_msgs/msg/pose.hpp>

#include <chrono>
#include <thread>
#include <vector>
#include <string>


int main(int argc, char** argv)
{
  // ============================================================
  // ROS 2 초기화
  // ============================================================

  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>(
      "pick_object",
      rclcpp::NodeOptions()
          .automatically_declare_parameters_from_overrides(true));


  // MoveGroupInterface가 통신할 수 있도록 executor 실행
  rclcpp::executors::SingleThreadedExecutor executor;

  executor.add_node(node);

  std::thread executor_thread([&executor]() {
    executor.spin();
  });


  // ============================================================
  // MoveIt Planning Group
  // ============================================================

  // Panda Arm
  moveit::planning_interface::MoveGroupInterface arm(
      node,
      "panda_arm");

  // Panda Gripper
  moveit::planning_interface::MoveGroupInterface hand(
      node,
      "hand");


  // ============================================================
  // 기본 Planning 설정
  // ============================================================

  arm.setPlanningTime(10.0);

  arm.setNumPlanningAttempts(10);

  arm.setMaxVelocityScalingFactor(0.2);

  arm.setMaxAccelerationScalingFactor(0.2);


  // ============================================================
  // STEP 1
  // Gripper Open
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "========================================");

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 1 : Opening gripper");

  hand.setNamedTarget("open");

  auto hand_result = hand.move();

  if (!static_cast<bool>(hand_result))
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "STEP 1 FAILED : Could not open gripper");

    executor.cancel();

    if (executor_thread.joinable())
    {
      executor_thread.join();
    }

    rclcpp::shutdown();

    return 1;
  }

  std::this_thread::sleep_for(
      std::chrono::seconds(1));


  // ============================================================
  // STEP 2
  // Pre-Grasp Pose
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 2 : Moving to pre-grasp pose");


  geometry_msgs::msg::Pose target_pose;


  // ------------------------------------------------------------
  // End Effector Orientation
  //
  // Gripper가 아래쪽을 향하도록 설정
  // Quaternion = 180 deg about X
  // ------------------------------------------------------------

  target_pose.orientation.x = 1.0;
  target_pose.orientation.y = 0.0;
  target_pose.orientation.z = 0.0;
  target_pose.orientation.w = 0.0;


  // ------------------------------------------------------------
  // Box 위치
  //
  // x = 0.45
  // y = 0.00
  // z = 0.05
  //
  // Box 위쪽 pre-grasp 위치
  // ------------------------------------------------------------

  target_pose.position.x = 0.45;
  target_pose.position.y = 0.00;
  target_pose.position.z = 0.30;


  arm.setPoseTarget(target_pose);


  auto arm_result = arm.move();


  if (!static_cast<bool>(arm_result))
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "STEP 2 FAILED : Could not reach pre-grasp pose");


    executor.cancel();

    if (executor_thread.joinable())
    {
      executor_thread.join();
    }

    rclcpp::shutdown();

    return 1;
  }


  arm.clearPoseTargets();


  std::this_thread::sleep_for(
      std::chrono::seconds(1));


  // ============================================================
  // STEP 3
  // Cartesian Approach
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 3 : Cartesian approach");


  // 현재 End Effector 위치를 가져옴
  geometry_msgs::msg::Pose current_pose =
      arm.getCurrentPose().pose;


  RCLCPP_INFO(
      node->get_logger(),
      "Current hand position: x=%.3f y=%.3f z=%.3f",
      current_pose.position.x,
      current_pose.position.y,
      current_pose.position.z);


  // 현재 위치를 복사
  geometry_msgs::msg::Pose approach_pose =
      current_pose;


  // ------------------------------------------------------------
  // 아래 방향으로 8 cm 이동
  //
  // z -= 0.08
  //
  // 즉 현재 위치에서 수직으로 내려감
  // ------------------------------------------------------------

  approach_pose.position.z -= 0.08;


  // Cartesian Waypoint
  std::vector<geometry_msgs::msg::Pose> waypoints;

  waypoints.push_back(approach_pose);


  // 계산된 trajectory가 저장될 변수
  moveit_msgs::msg::RobotTrajectory approach_trajectory;


  // End Effector 이동 계산 간격
  // 0.01 m = 1 cm
  const double eef_step = 0.01;


  // ------------------------------------------------------------
  // Cartesian Path 계산
  //
  // 반환값:
  //
  // 1.0 = 100%
  // 0.5 = 50%
  // 0.0 = 실패
  // ------------------------------------------------------------

  double fraction =
      arm.computeCartesianPath(
          waypoints,
          eef_step,
          approach_trajectory,
          true);


  RCLCPP_INFO(
      node->get_logger(),
      "Cartesian approach achieved: %.1f%%",
      fraction * 100.0);


  // 95% 미만이면 실패로 판단
  if (fraction < 0.95)
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "STEP 3 FAILED : Cartesian approach incomplete");


    executor.cancel();

    if (executor_thread.joinable())
    {
      executor_thread.join();
    }

    rclcpp::shutdown();

    return 1;
  }


  // ------------------------------------------------------------
  // 계산된 Cartesian trajectory 실행
  // ------------------------------------------------------------

  auto execute_result =
      arm.execute(approach_trajectory);


  if (!static_cast<bool>(execute_result))
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "STEP 3 FAILED : Could not execute Cartesian trajectory");


    executor.cancel();

    if (executor_thread.joinable())
    {
      executor_thread.join();
    }

    rclcpp::shutdown();

    return 1;
  }


  RCLCPP_INFO(
      node->get_logger(),
      "STEP 3 SUCCESS : Approach completed");


  std::this_thread::sleep_for(
      std::chrono::seconds(1));


  // ============================================================
  // STEP 4
  // Gripper Close
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 4 : Closing gripper");


  hand.setNamedTarget("close");


  hand_result = hand.move();


  if (!static_cast<bool>(hand_result))
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "STEP 4 FAILED : Could not close gripper");


    executor.cancel();

    if (executor_thread.joinable())
    {
      executor_thread.join();
    }

    rclcpp::shutdown();

    return 1;
  }


  std::this_thread::sleep_for(
      std::chrono::seconds(1));


  // ============================================================
  // STEP 5
  // Attach Object
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 5 : Attaching pick_box");


  // Object와 접촉해도 되는 Robot Link
  std::vector<std::string> touch_links = {

      "panda_hand",

      "panda_leftfinger",

      "panda_rightfinger"

  };


  bool attach_success =
      arm.attachObject(
          "pick_box",
          "panda_hand",
          touch_links);


  if (!attach_success)
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "STEP 5 FAILED : Attach failed");


    executor.cancel();

    if (executor_thread.joinable())
    {
      executor_thread.join();
    }

    rclcpp::shutdown();

    return 1;
  }


  RCLCPP_INFO(
      node->get_logger(),
      "STEP 5 SUCCESS : pick_box attached");


  std::this_thread::sleep_for(
      std::chrono::seconds(2));


  // ============================================================
  // STEP 6
  // Cartesian Lift
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "STEP 6 : Lifting object");


  // Attach 이후 현재 위치 다시 읽기
  geometry_msgs::msg::Pose lift_pose =
      arm.getCurrentPose().pose;


  // 위로 10 cm
  lift_pose.position.z += 0.10;


  std::vector<geometry_msgs::msg::Pose> lift_waypoints;

  lift_waypoints.push_back(lift_pose);


  moveit_msgs::msg::RobotTrajectory lift_trajectory;


  double lift_fraction =
      arm.computeCartesianPath(
          lift_waypoints,
          eef_step,
          lift_trajectory,
          true);


  RCLCPP_INFO(
      node->get_logger(),
      "Cartesian lift achieved: %.1f%%",
      lift_fraction * 100.0);


  if (lift_fraction < 0.95)
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "STEP 6 FAILED : Cartesian lift incomplete");


    executor.cancel();

    if (executor_thread.joinable())
    {
      executor_thread.join();
    }

    rclcpp::shutdown();

    return 1;
  }


  execute_result =
      arm.execute(lift_trajectory);


  if (!static_cast<bool>(execute_result))
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "STEP 6 FAILED : Could not execute lift trajectory");


    executor.cancel();

    if (executor_thread.joinable())
    {
      executor_thread.join();
    }

    rclcpp::shutdown();

    return 1;
  }


  // ============================================================
  // 성공
  // ============================================================

  RCLCPP_INFO(
      node->get_logger(),
      "========================================");


  RCLCPP_INFO(
      node->get_logger(),
      "PICK SUCCESS");


  RCLCPP_INFO(
      node->get_logger(),
      "Object was opened, approached, closed, attached and lifted");


  RCLCPP_INFO(
      node->get_logger(),
      "========================================");


  // ============================================================
  // 종료
  // ============================================================

  executor.cancel();


  if (executor_thread.joinable())
  {
    executor_thread.join();
  }


  rclcpp::shutdown();


  return 0;
}
