#include <rclcpp/rclcpp.hpp>

#include <moveit/planning_scene_interface/planning_scene_interface.hpp>
#include <moveit_msgs/msg/collision_object.hpp>

#include <shape_msgs/msg/solid_primitive.hpp>
#include <geometry_msgs/msg/pose.hpp>

#include <chrono>
#include <thread>

int main(int argc, char** argv)
{
  // ROS 2 초기화
  rclcpp::init(argc, argv);

  // 노드 생성
  auto node = std::make_shared<rclcpp::Node>("add_object");

  // Planning Scene에 접근하기 위한 인터페이스
  moveit::planning_interface::PlanningSceneInterface planning_scene_interface;

  // Collision Object 생성
  moveit_msgs::msg::CollisionObject object;

  // 물체 위치의 기준 좌표계
  object.header.frame_id = "panda_link0";

  // 물체 ID
  object.id = "pick_box";

  // 박스 형상 생성
  shape_msgs::msg::SolidPrimitive primitive;
  primitive.type = shape_msgs::msg::SolidPrimitive::BOX;

  // BOX 크기
  // x = 5 cm
  // y = 5 cm
  // z = 10 cm
  primitive.dimensions = {
    0.05,
    0.05,
    0.10
  };

  // 박스 위치
  geometry_msgs::msg::Pose pose;

  // 회전 없음
  pose.orientation.x = 0.0;
  pose.orientation.y = 0.0;
  pose.orientation.z = 0.0;
  pose.orientation.w = 1.0;

  // Panda 앞쪽 바닥
  pose.position.x = 0.45;
  pose.position.y = 0.00;

  // 박스 높이가 0.10 m이므로
  // 중심을 0.05 m에 놓으면 바닥에 닿음
  pose.position.z = 0.05;

  // Object에 형상과 위치 등록
  object.primitives.push_back(primitive);
  object.primitive_poses.push_back(pose);

  // Planning Scene에 ADD
  object.operation = moveit_msgs::msg::CollisionObject::ADD;

  // 실제 적용
  bool success =
      planning_scene_interface.applyCollisionObject(object);

  if (success)
  {
    RCLCPP_INFO(
      node->get_logger(),
      "pick_box added successfully"
    );
  }
  else
  {
    RCLCPP_ERROR(
      node->get_logger(),
      "Failed to add pick_box"
    );
  }

  // Scene 반영 시간 확보
  std::this_thread::sleep_for(
      std::chrono::seconds(1)
  );

  rclcpp::shutdown();
  return 0;
}
