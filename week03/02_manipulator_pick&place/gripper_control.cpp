#include <rclcpp/rclcpp.hpp>
#include <moveit/move_group_interface/move_group_interface.hpp>

#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
  // ROS 2 초기화
  rclcpp::init(argc, argv);

  // ROS 2 Node 생성
  auto node = std::make_shared<rclcpp::Node>(
      "gripper_control",
      rclcpp::NodeOptions()
          .automatically_declare_parameters_from_overrides(true));

  // MoveIt MoveGroupInterface 생성
  // "hand" = Panda gripper planning group
  moveit::planning_interface::MoveGroupInterface hand(node, "hand");

  RCLCPP_INFO(node->get_logger(), "Opening gripper...");

  // SRDF에 정의되어 있는 named state "open"
  hand.setNamedTarget("open");

  // 계획 + 실행
  bool success = static_cast<bool>(hand.move());

  if (!success)
  {
    RCLCPP_ERROR(node->get_logger(), "Failed to open gripper");
    rclcpp::shutdown();
    return 1;
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));

  RCLCPP_INFO(node->get_logger(), "Closing gripper...");

  // Panda hand의 닫힌 자세
  hand.setNamedTarget("close");

  success = static_cast<bool>(hand.move());

  if (!success)
  {
    RCLCPP_ERROR(node->get_logger(), "Failed to close gripper");
    rclcpp::shutdown();
    return 1;
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));

  RCLCPP_INFO(node->get_logger(), "Opening gripper again...");

  hand.setNamedTarget("open");
  hand.move();

  RCLCPP_INFO(node->get_logger(), "Finished");

  rclcpp::shutdown();
  return 0;
}

