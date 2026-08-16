# 매니퓰레이터를 불러와서 Moveit or Moveit2 (for ROS2) tutorial 을 따라하면서, pick and place 구현하기

## MoveIt2 튜토리얼
 : ROS 2용 로봇 Manipulation / Motion Planning Framework  
 motion planning, kinematics, collision checking 등을 제공  
 **planning group**
 : moveit이 한 그룹으로 제어할 joint 집합  
 **end effertor**
 로봇 팔 끝에서 실제로 작업하는 부분 ex)girpper  
 **forward kinematics**
 joint angle을 알 때 end effector 위치를 계산  
 **inverse kinematics**
 FK의 반대  
 *gripper를 x = 0.45, y = 0.10, z = 0.30 에 보내고 싶을 때 IK로 가능한 Joint angle을 찾을 수 있다.  
 **motion planning**: IK가 찾은 Joint 값을 토대로, 그 지점까지 어떤 경로로 갈 것인지 찾음
 **planning scene**: 로봇 상태와 world의 물체 정보를 저장(collision checking과 motion planning에 사용됨)


1> 준비
MoveIt2, MoveIt2_tutorials를 설치하고 Franka Panda 매니퓰레이터를 RViz에 띄우기.
```
</> Bash
$ mkdir -p ~/ws_moveit/src
$ cd ~/ws_moveit/src
$ git clone https://github.com/moveit/moveit2_tutorials.git

//movelt tutorial workspace 만들고 공식 tutorial repository를 clone
```

```
</> bash
$ cd ~/ws_moveit
$ colcon build --symlink-install

$ source ~/ws_moveit/install/setup.bash

// ROS2 workplace 빌드

$ source /opt/ros/jazzy/setup.bash
$ source ~/ws_moveit/install/setup.bash

ros2 launch moveit_resources_panda_moveit_config demo.launch.py

// RViz에 Franka Panda 매니퓰레이터 생성
```
<img width="1854" height="1048" alt="image" src="https://github.com/user-attachments/assets/717ee113-535a-4b34-b8d7-0641ae34438d" />

* Xacro
  ROS의 URDF를 더 편하게 작성하기 위한 매크로 시스템
  예를 들어 로봇의 link, joint를 반복해서 작성하거나 특정 설정값을 parameter로 넘길 때 사용
* RViz
  ROS2 의 시각화 도구
  MoveIt 내부에서 계산된 로봇 상태와 경로를 직접 눈으로 볼 수 있음
  Current / Scene Robot : 현재 로봇 자세
  Start State: Motion Planning이 시작되는 로봇 자세
  Goal State : 목표 자세
  Planned Path : Start에서 Goal까지 MoveIt이 계산한 경로

  2> RViz에서 실제로 Panda를 움직여보기
  
  ***1. MotionPlanning 패널 확인***
```
   Displays
   >MotionPlanning
   >Planning Group
    Start State
    Goal State
    Planning
    Planned Path
  ```
<img width="498" height="546" alt="image" src="https://github.com/user-attachments/assets/c81a8148-3670-4ed9-9348-a45612631027" />

  ***2. Planning Group 확인***
  planning group : MoveIt이 '어떤 joint 묶음을 같이 움직일 것인가'를 정의한 것.\
   panda_arm 선택 -> Panda의 7개 arm joint를 하나의 그룹으로 움직임.

  ***3. End Effector의 Interactive Marker 직접 움직여보기***
  : Goal State를 수정해보기, IK

| 원래 state | IK solve success |
| --- | --- |
| <img src="https://github.com/user-attachments/assets/e30b65a2-ea3a-4926-b6c6-5be89bfacb2f" width="100%" /> | <img src="https://github.com/user-attachments/assets/8abc6e6c-0178-4f8e-ac6b-bd8fb5b8c784" width="100%" /> |


| 유효하지 않은 Goal State 설정 | IK solve fail |
| --- | --- |
| <img width="520" height="490" alt="image" src="https://github.com/user-attachments/assets/113c1336-38fd-4373-96f7-fb13be718c63" /> | <img width="530" height="444" alt="image" src="https://github.com/user-attachments/assets/c0be71a7-c8a9-495a-a741-5e4956591cbd" />
|

| 유효한 Goal State 설정 | plan success | execute success |
| --- | --- | --- |
| <img width="733" height="492" alt="image" src="https://github.com/user-attachments/assets/c760e8c3-bebe-4ea5-adb4-e6f95ffab968" /> | ![plan success](Screencast%20from%202026-08-16%2016-32-43.gif) | ![execute success](Screencast%20from%202026-08-16%2016-52-03.gif) |


```
  MotionPlanning>Plan
   Current State -> Goal State -> Motion Planner -> 충돌 검사 -> Trajectory 생성

```
```
</> bash
$ source /opt/ros/jazzy/setup.bash
$ source ~/ws_moveit/install/setup.bash
$ ros2 launch moveit_resources_panda_moveit_config demo.launch.py

//Franka Panda 매니퓰레이터 실행
```
```
Planning request received
...
Planning succeeded

//터미널 로그
```
* Plan → 경로 계산 및 미리보기
  Execute → 계산한 경로 실제 적용
* MoveIt은 Planning Scene의 RobotState와 collision 정보를 이용해 충돌 여부를 검사한다.

  ***4. ROS 2에서 현재 Joint State 보기***
```
  </> bash
  $ source /opt/ros/jazzy/setup.bash
  $ ros2 topic list | grep joint
  $ ros2 topic echo /joint_states

  //RViz에서 Panda를 Execute 하면, /joint_states 숫자가 바뀜.
```
---------------------------------------------------------------
## Pick&Place
***1. 집을 물체를 Planning Scene에 추가하기***

```
// C++
#include <rclcpp/rclcpp.hpp>

#include <moveit/planning_scene_interface/planning_scene_interface.hpp>

#include <moveit_msgs/msg/collision_object.hpp>
#include <shape_msgs/msg/solid_primitive.hpp>
#include <geometry_msgs/msg/pose.hpp>

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);

  auto node = rclcpp::Node::make_shared("add_collision_object");

  moveit::planning_interface::PlanningSceneInterface planning_scene_interface;

  moveit_msgs::msg::CollisionObject object;

  // 기준 좌표계
  object.header.frame_id = "panda_link0";

  // Planning Scene에서 물체를 구분하기 위한 이름
  object.id = "pick_box";

  // 물체 형상
  shape_msgs::msg::SolidPrimitive primitive;
  primitive.type = shape_msgs::msg::SolidPrimitive::BOX;

  // x, y, z [m]
  primitive.dimensions = {0.05, 0.05, 0.10};

  // 물체 위치
  geometry_msgs::msg::Pose pose;
  pose.orientation.w = 1.0;

  pose.position.x = 0.45;
  pose.position.y = 0.0;
  pose.position.z = 0.05;

  object.primitives.push_back(primitive);
  object.primitive_poses.push_back(pose);

  object.operation = moveit_msgs::msg::CollisionObject::ADD;

  planning_scene_interface.applyCollisionObject(object);

  RCLCPP_INFO(node->get_logger(), "Added pick_box to Planning Scene");

  rclcpp::shutdown();
  return 0;
}
```

```
moveit_msgs::msg::CollisionObject object;

//MoveIt에게 “가상 공간에 물체 하나 있다.” 라고 알려주는 메시지.
```
```
object.header.frame_id = "panda_link0";

//위치 기준 좌표계.
x = 0.45
y = 0.0
z = 0.05
가 Panda base 좌표계 기준이라는 뜻.
```
```
primitive.type = shape_msgs::msg::SolidPrimitive::BOX;

//형상


primitive.dimensions = {0.05, 0.05, 0.10};


//크기
```

<img width="545" height="472" alt="image" src="https://github.com/user-attachments/assets/0fff8047-aaba-4b88-8e9a-ca404013f8cd" />
-> 빌드 및 실행 결과 5 cm × 5 cm × 10 cm 박스를 생성했다!

***2. Gripper 추가하기***

***3. Attach & Detach***

***4. MTC***
  

  
  
