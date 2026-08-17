#include <rclcpp/rclcpp.hpp>
#include <moveit/move_group_interface/move_group_interface.hpp>

#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>(
      "attach_object",
      rclcpp::NodeOptions()
          .automatically_declare_parameters_from_overrides(true));

  moveit::planning_interface::MoveGroupInterface arm(node, "panda_arm");

  RCLCPP_INFO(node->get_logger(), "Attaching pick_box...");

  arm.attachObject("pick_box", "panda_hand");

  std::this_thread::sleep_for(std::chrono::seconds(3));

  RCLCPP_INFO(node->get_logger(), "pick_box attached.");

  rclcpp::shutdown();
  return 0;
}

