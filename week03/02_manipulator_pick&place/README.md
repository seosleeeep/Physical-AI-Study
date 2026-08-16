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

~opening~
최종 목표는
(1)접근 → (2)잡기 → (3)attach → (4)lift → (5)이동 → (6)내려놓기 → (7)open + Detach
과정을 구현하는 것이다. 

이는 MoveIt 관점에서,

Planning Scene에 Object 추가
        ↓
Open Gripper
        ↓
Object로 이동
        ↓
Approach
        ↓
Close Gripper
        ↓
Attach Object
        ↓
Lift
        ↓
Place 위치로 이동
        ↓
Lower
        ↓
Open Gripper
        ↓
Detach Object
        ↓
Retreat

이다.

**planning Scene**
: MoveIt이 알고 있는 가상 세계이다.
MoveIt은 현실을 직접 볼 수 없으므로 로봇 있음, 상자 있음, 테이블 있음 등의 정보를 알려줘야 하는데, 이 정보를 가지고 있는 것이다. Planning Scene에 collision object를 넣어서 로봇이 주변 물체를 고려하도록 한다. (Robot, Object, Obstacle, Attached Object, Collision 정보 가 들어있다.)

**Collision Object**
: 이름에 써진 collision -> MoveIt이 이 물체와 충돌하면 안 된다는 걸 의미한다.

```
</> C++
moveit_msgs::msg::CollisionObject object;
//'MoveIt 세계에 넣을 물체 하나 만들겠다' 선언
```

```
</> C++
object.id = "pick_box";
//물체 이름. 
```
```
</> C++
shape_msgs::msg::SolidPrimitive primitive;
primitive.type = shape_msgs::msg::SolidPrimitive::BOX;
//shape를 만드는 부분, 물체의 기하학적 형상을 정의, 상자 생성 (BOX, SPHERE, CYLINDER, CONE 등의 primitive geomery가 있음.)
```
```
</> C++
primitive.dimensions = {0.05, 0.05, 0.10};
//dimension을 정의
```
```
</> C++
geometry_msgs::msg::Pose pose;
//pose를 정의 (pose는 position+orientation이다. x, y, z, rotation을 가진다.)
```
```
</> C++
object.primitives.push_back(primitive);
object.primitive_poses.push_back(pose);
//Object에 Shape와 Pose를 넣는다. 모양은 primitive, 위치는 pose.
```
```
</> C++
object.operation =
    moveit_msgs::msg::CollisionObject::ADD;
//Planning Scene에 추가. (삭제는 REMOVE)

moveit::planning_interface::PlanningSceneInterface planning_scene_interface;
//PlanningSceneInterface는 우리 프로그램이 MoveIt Planning Scene과 통신할 수 있게 해주는 인터페이스

planning_scene_interface.applyCollisionObject(object);
//C++ Node -> PlanningSceneInterface -> move_group / Planning Scene -> pick_box 추가
```




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
```
</> Bash
nano ~/ws_moveit/src/panda_pick_place/src/gripper_control.cpp

</> C++
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

// gripper 제어 코드

</> Bash
nano ~/ws_moveit/src/panda_pick_place/CMakeLists.txt

</> cmake
add_executable(gripper_control src/gripper_control.cpp)

ament_target_dependencies(gripper_control
  rclcpp
  moveit_ros_planning_interface
)

//기존 add_object 아래에 추가할 내용.

</> cmake
install(TARGETS
  add_object
  gripper_control
  DESTINATION lib/${PROJECT_NAME}
)
//기존 install()에 추가할 내용.
```
```
</> Bash
source /opt/ros/jazzy/setup.bash
source ~/ws_moveit/install/setup.bash

cd ~/ws_moveit

colcon build --symlink-install \
  --packages-select panda_pick_place
//빌드
```

<img width="626" height="627" alt="image" src="https://github.com/user-attachments/assets/63a2ddbc-432a-4501-b3a9-4e7c8b701190" />
-> gripper 생성을 완료한 모습.

ros2 run panda_pick_place gripper_control 실행하면,
RViz에서, OPEN -> CLOSE -> OPEN 순서로 움직임.

![execute success](Screencast%20from%202026-08-16%2018-41-12.gif)






***3. Attach & Detach***

***4. MTC***
  

  
  
