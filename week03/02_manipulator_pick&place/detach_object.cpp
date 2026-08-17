#include <rclcpp/rclcpp.hpp>
#include <moveit/move_group_interface/move_group_interface.hpp>

#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>(
      "detach_object",
      rclcpp::NodeOptions()
          .automatically_declare_parameters_from_overrides(true));

  moveit::planning_interface::MoveGroupInterface arm(
      node,
      "panda_arm");

  RCLCPP_INFO(
      node->get_logger(),
      "Detaching pick_box...");

  bool success = arm.detachObject("pick_box");

  if (success)
  {
    RCLCPP_INFO(
        node->get_logger(),
        "pick_box detached successfully");
  }
  else
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "Failed to detach pick_box");
  }

  std::this_thread::sleep_for(
      std::chrono::seconds(2));

  rclcpp::shutdown();

  return 0;
}

