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
1> Gripper 제어 파일 만들기
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

// 
```
2> CMakeLists 수정
3> 빌드
4> 실행
source /opt/ros/jazzy/setup.bash
source ~/ws_moveit/install/setup.bash

 Opening gripper...
 Closing gripper...
 Opening gripper again...
 Finished

![execute success](week03/02_manipulator_pick%26place/Screencast%20from%202026-08-16%2018-41-12.gif)


***3. Attatch***

1> 목표
: Planning Scene에 추가한 `pick_box`를 Panda 로봇의 gripper에 연결하여, 로봇이 움직일 때 물체가 함께 움직이도록 구현하자 !

전체 과정은 다음과 같다.

```text
Collision Object 생성
        ↓
Planning Scene에 Object 추가
        ↓
Object 위치를 Gripper 근처로 설정
        ↓
panda_hand에 Object Attach
        ↓
Attached Collision Object로 변경
        ↓
Panda가 움직이면 Object도 함께 이동
```

---

2> Attach 전후 결과 비교

| Object가 Gripper에 제대로 붙지 않은 상태 | Object가 Gripper에 제대로 붙은 상태 |
| --- | --- |
| <img width="300" height="315" alt="Attach 전" src="https://github.com/user-attachments/assets/59d944d5-6d29-4eae-bd69-d372e10ba696" /> | <img width="300" height="315" alt="Attach 후" src="https://github.com/user-attachments/assets/0e2d1c5a-d276-49db-8fbf-35261c3570cd" /> |
| Object가 gripper와 떨어진 위치에서 Attach되어 시각적으로 붙어 있지 않음                                                                                     | Object 위치를 `panda_hand` 근처로 설정한 후 Attach하여 gripper와 함께 보임                                                                             |

### 첫 번째 상태에서 발생한 문제

처음에는 `attachObject()` 자체를 실행했지만, `pick_box`의 위치가 gripper와 멀리 떨어져 있었다.

중요한 점은 **Attach 명령이 물체를 자동으로 gripper 위치로 순간이동시키는 명령은 아니라는 것**이다.

즉,

```text
Box 위치

□


                  Gripper
                     │
                     │

attachObject()
```

를 실행한다고 해서 자동으로

```text
Gripper
   │
   □
```

가 되는 것은 아니다.

Attach는 현재 Object와 Robot Link 사이의 관계를 Planning Scene에서 변경하는 기능이다.

따라서 물체를 실제로 잡고 있는 것처럼 표현하려면 Attach 전에 Object가 gripper 근처에 위치해야 한다.

---

3> 주요 개념

### ★ Planning Scene

*Planning Scene은 MoveIt이 알고 있는 로봇 주변의 가상 환경이다.

다음과 같은 정보를 포함한다.

```text
Planning Scene
│
├── Robot State
├── Collision Object
├── Attached Collision Object
├── 장애물
└── Collision 정보
```

MoveIt은 이 정보를 이용하여 로봇의 경로를 계획하고 충돌을 검사한다.

---

### ★ Collision Object

*Collision Object는 Planning Scene 안에 존재하는 물체 또는 장애물을 의미한다.

이번 실습에서는 다음과 같은 Box를 생성하였다.

```text
ID   : pick_box
Shape: BOX
Size : 0.05 × 0.05 × 0.10 m
```

즉,

```text
5 cm × 5 cm × 10 cm
```

크기의 직육면체이다.

---

### ★ World Object

Attach하기 전의 `pick_box`는 로봇과 독립된 *World Object이다.

```text
Planning Scene

Panda                  pick_box
  │                       □
```

따라서 Panda가 움직여도 Box는 움직이지 않는다.

---

### ★ Attached Collision Object

Object를 Robot Link에 Attach하면 해당 물체는 *Attached Collision Object가 된다.

```text
Panda
  │
panda_hand
  │
pick_box
  □
```

이제 MoveIt은 `pick_box`를 로봇이 들고 있는 물체로 취급한다.

따라서 Panda가 움직이면 Box도 같이 움직인다.

---

### ★ panda_hand

`panda_hand`는 Panda gripper 부분의 Link이다.

이번 실습에서는 다음과 같이 `pick_box`를 `panda_hand`에 연결하였다.

```cpp
arm.attachObject("pick_box", "panda_hand");
```

의미는 다음과 같다.

```text
pick_box
    ↓
panda_hand에 Attach
    ↓
Panda와 함께 이동
```

---

### ★ touch_links

Gripper가 물체를 잡으면 손가락과 물체가 서로 접촉해야 한다.

하지만 MoveIt에서는 기본적으로 Robot과 Collision Object가 접촉하면 충돌로 판단할 수 있다.

따라서 다음 두 finger link와 Object 사이의 접촉을 허용하였다.

```text
panda_leftfinger
panda_rightfinger
```

즉,

```text
일반 상태

Robot ↔ Object
Collision 금지


Grasp 상태

Finger ↔ Object
접촉 허용
```

이 필요하다.

---

4> Panda Hand 위치 확인
처음에는 Object 위치와 Gripper 위치가 서로 달랐기 때문에 Attach가 시각적으로 이상하게 보였다.

따라서 TF를 이용하여 `panda_hand` 위치를 확인하였다.

```bash
source /opt/ros/jazzy/setup.bash
source ~/ws_moveit/install/setup.bash

ros2 run tf2_ros tf2_echo panda_link0 panda_hand
```

측정 결과:

```text
Translation:
x = 0.643
y = -0.010
z = 0.605
```

즉 `panda_link0` 좌표계에서 현재 `panda_hand`의 위치는 다음과 같다.

```text
panda_hand

x = 0.643 m
y = -0.010 m
z = 0.605 m
```

---

5> Object 위치 수정

`panda_hand`와 완전히 같은 위치에 Box 중심을 생성하면 Object가 손 내부에 겹칠 수 있다.

따라서 Box 높이가 `0.10 m`인 것을 고려하여 중심을 약 5 cm 아래에 배치하였다.

```cpp
pose.position.x = 0.643;
pose.position.y = -0.010;
pose.position.z = 0.555;
```

관계는 다음과 같다.

```text
panda_hand
   ●  z = 0.605
   │
   │
  ┌─┐
  │ │
  │□│  pick_box
  │ │
  └─┘
   ●  Box 중심
      z = 0.555
```

---

6> Collision Object 생성 코드

파일 위치:

```text
~/ws_moveit/src/panda_pick_place/src/add_object.cpp
```

코드:

```cpp
#include <rclcpp/rclcpp.hpp>

#include <moveit/planning_scene_interface/planning_scene_interface.hpp>

#include <moveit_msgs/msg/collision_object.hpp>
#include <shape_msgs/msg/solid_primitive.hpp>
#include <geometry_msgs/msg/pose.hpp>

int main(int argc, char** argv)
{
  // ROS 2 초기화
  rclcpp::init(argc, argv);

  // ROS 2 Node 생성
  auto node = rclcpp::Node::make_shared("add_collision_object");

  // MoveIt Planning Scene에 접근하기 위한 Interface
  moveit::planning_interface::PlanningSceneInterface
      planning_scene_interface;

  // Collision Object 메시지 생성
  moveit_msgs::msg::CollisionObject object;

  // Object 위치의 기준 좌표계
  object.header.frame_id = "panda_link0";

  // Object를 구분하기 위한 ID
  object.id = "pick_box";

  // Object 형상 생성
  shape_msgs::msg::SolidPrimitive primitive;

  // 직육면체 BOX 사용
  primitive.type =
      shape_msgs::msg::SolidPrimitive::BOX;

  // Box 크기
  // X = 5 cm
  // Y = 5 cm
  // Z = 10 cm
  primitive.dimensions = {
      0.05,
      0.05,
      0.10
  };

  // Object 위치 및 방향
  geometry_msgs::msg::Pose pose;

  // Quaternion 기준 회전 없음
  pose.orientation.w = 1.0;

  // panda_hand 근처에 Object 배치
  pose.position.x = 0.643;
  pose.position.y = -0.010;
  pose.position.z = 0.555;

  // Collision Object에 형상 추가
  object.primitives.push_back(primitive);

  // Collision Object에 Pose 추가
  object.primitive_poses.push_back(pose);

  // Planning Scene에서 수행할 동작
  // ADD = Object 추가
  object.operation =
      moveit_msgs::msg::CollisionObject::ADD;

  // Object를 실제 Planning Scene에 적용
  planning_scene_interface.applyCollisionObject(object);

  RCLCPP_INFO(
      node->get_logger(),
      "Added pick_box to Planning Scene"
  );

  rclcpp::shutdown();

  return 0;
}
```

---

# 7. `operation = ADD`와 `applyCollisionObject()` 차이

두 부분은 서로 역할이 다르다.

```cpp
object.operation =
    moveit_msgs::msg::CollisionObject::ADD;
```

이 부분은:

```text
이 CollisionObject에 대해
어떤 작업을 할 것인가?
        ↓
ADD
```

라고 **작업 종류를 지정**하는 것이다.

아직 Planning Scene에 전달된 것은 아니다.

반면,

```cpp
planning_scene_interface.applyCollisionObject(object);
```

는 완성된 Collision Object 메시지를 **실제로 Planning Scene에 적용**한다.

따라서 전체 과정은:

```text
CollisionObject 생성
       ↓
ID 지정
       ↓
Shape 지정
       ↓
Pose 지정
       ↓
operation = ADD
       ↓
"추가 작업"이라고 표시
       ↓
applyCollisionObject()
       ↓
Planning Scene에 실제 추가
```

이다.

---

# 8. Attach Object 코드

파일 위치:

```text
~/ws_moveit/src/panda_pick_place/src/attach_object.cpp
```

코드:

```cpp
#include <rclcpp/rclcpp.hpp>

#include <moveit/move_group_interface/move_group_interface.hpp>

#include <chrono>
#include <thread>
#include <vector>
#include <string>

int main(int argc, char** argv)
{
  // ROS 2 초기화
  rclcpp::init(argc, argv);

  // ROS 2 Node 생성
  auto node = std::make_shared<rclcpp::Node>(
      "attach_object",
      rclcpp::NodeOptions()
          .automatically_declare_parameters_from_overrides(true)
  );

  // Panda Arm의 MoveGroupInterface 생성
  moveit::planning_interface::MoveGroupInterface
      arm(node, "panda_arm");

  // Object와 접촉을 허용할 Link 목록
  std::vector<std::string> touch_links;

  // Panda gripper의 왼쪽 finger
  touch_links.push_back("panda_leftfinger");

  // Panda gripper의 오른쪽 finger
  touch_links.push_back("panda_rightfinger");

  RCLCPP_INFO(
      node->get_logger(),
      "Attaching pick_box to panda_hand..."
  );

  // pick_box를 panda_hand에 Attach
  bool success = arm.attachObject(
      "pick_box",
      "panda_hand",
      touch_links
  );

  // 결과 확인
  if (success)
  {
    RCLCPP_INFO(
        node->get_logger(),
        "Attach request succeeded."
    );
  }
  else
  {
    RCLCPP_ERROR(
        node->get_logger(),
        "Attach request failed."
    );
  }

  // Planning Scene 반영을 확인하기 위해 잠시 대기
  std::this_thread::sleep_for(
      std::chrono::seconds(3)
  );

  // ROS 2 종료
  rclcpp::shutdown();

  return 0;
}
```

---

# 9. Attach 코드의 핵심

가장 중요한 부분은 다음 코드이다.

```cpp
arm.attachObject(
    "pick_box",
    "panda_hand",
    touch_links
);
```

각 인자의 의미는:

```text
"pick_box"
    ↓
Attach할 Object ID

"panda_hand"
    ↓
Object를 연결할 Robot Link

touch_links
    ↓
Object와 접촉해도 되는 Link 목록
```

이다.

전체 관계는:

```text
Planning Scene

pick_box
   │
   │ attachObject()
   ▼
panda_hand
   │
   ├── panda_leftfinger
   └── panda_rightfinger
```

가 된다.

---

# 10. CMakeLists.txt

이번 실습에서 사용한 실행 파일은:

```text
add_object
gripper_control
attach_object
```

이다.

따라서 `CMakeLists.txt`에는 각 C++ 파일을 executable로 등록해야 한다.

```cmake
add_executable(add_object
  src/add_object.cpp
)

ament_target_dependencies(add_object
  rclcpp
  moveit_ros_planning_interface
  moveit_msgs
  shape_msgs
  geometry_msgs
)


add_executable(gripper_control
  src/gripper_control.cpp
)

ament_target_dependencies(gripper_control
  rclcpp
  moveit_ros_planning_interface
)


add_executable(attach_object
  src/attach_object.cpp
)

ament_target_dependencies(attach_object
  rclcpp
  moveit_ros_planning_interface
)


install(TARGETS
  add_object
  gripper_control
  attach_object
  DESTINATION lib/${PROJECT_NAME}
)
```

### ★ add_executable()

```cmake
add_executable(
  attach_object
  src/attach_object.cpp
)
```

C++ 소스 파일을 ROS 2에서 실행할 수 있는 executable target으로 만든다.

### ★ ament_target_dependencies()

```cmake
ament_target_dependencies(
  attach_object
  rclcpp
  moveit_ros_planning_interface
)
```

해당 executable이 사용하는 ROS 2 / MoveIt 라이브러리를 연결한다.

### ★ install(TARGETS)

```cmake
install(TARGETS
  attach_object
  DESTINATION lib/${PROJECT_NAME}
)
```

빌드된 실행 파일을 ROS 2에서:

```bash
ros2 run panda_pick_place attach_object
```

형태로 실행할 수 있도록 설치한다.

---

# 11. 빌드

코드를 수정한 뒤:

```bash
cd ~/ws_moveit

colcon build \
  --symlink-install \
  --packages-select panda_pick_place
```

빌드 완료 후 반드시 workspace를 다시 source한다.

```bash
source /opt/ros/jazzy/setup.bash
source ~/ws_moveit/install/setup.bash
```

---

# 12. 실행 순서

## Terminal 1 - Panda MoveIt 실행

```bash
source /opt/ros/jazzy/setup.bash
source ~/ws_moveit/install/setup.bash

ros2 launch moveit_resources_panda_moveit_config demo.launch.py
```

RViz와 Panda Robot을 실행한다.

---

## Terminal 2 - Object 추가

```bash
source /opt/ros/jazzy/setup.bash
source ~/ws_moveit/install/setup.bash

ros2 run panda_pick_place add_object
```

이 명령을 실행하면 `pick_box`가 Planning Scene에 추가된다.

---

## Terminal 2 - Object Attach

```bash
ros2 run panda_pick_place attach_object
```

정상적으로 실행되면:

```text
Attaching pick_box to panda_hand...
Attach request succeeded.
```

와 같은 로그를 확인할 수 있다.

---

# 13. Attach 전후 Planning Scene 변화

### Attach 전

```text
Planning Scene

Robot                    World Object

Panda                       □
                            │
                         pick_box
```

이때 Box는 Robot과 독립적이다.

Panda가 움직여도 Box는 움직이지 않는다.

---

### Attach 후

```text
Planning Scene

Panda
  │
panda_hand
  │
  □
pick_box
```

이제 `pick_box`는 Robot에 연결된 *Attached Collision Object이다.

Panda가 움직이면 Box도 함께 움직인다.

---

# 14. 이번 실습에서 중요한 점

### ★ Attach는 물체를 순간이동시키는 기능이 아니다.

처음 실습에서 Box가 바닥에 있는 상태로:

```cpp
attachObject()
```

를 실행했지만 Box가 gripper 위치로 이동하지 않았다.

Attach는:

```text
Object 위치 변경
```

기능이 아니라,

```text
Object의 소속 변경
```

기능이기 때문이다.

즉:

```text
World Object

      ↓ Attach

Robot Attached Object
```

로 변경하는 것이다.

---

### ★ 실제 Pick에서는 Attach 전에 접근 과정이 필요하다.

이번 실습에서는 Attach 기능 자체를 확인하기 위해 Box를 미리 gripper 근처에 생성했다.

하지만 실제 Pick 과정에서는 다음 순서가 필요하다.

```text
Object가 바닥에 존재
        ↓
Gripper Open
        ↓
Panda가 Object 근처로 이동
        ↓
Approach
        ↓
Gripper Close
        ↓
Attach Object
        ↓
Lift
```

따라서 이번 Attach 실습은 **Pick & Place 전체 과정 중 Attach 기능을 독립적으로 확인하기 위한 실습**이다.

---

# 15. 현재까지 구현한 기능

```text
Ubuntu 24.04
    ↓
ROS 2 Jazzy
    ↓
MoveIt 2
    ↓
Panda Manipulator 실행
    ↓
RViz Motion Planning
    ↓
Collision Object 생성 ✅
    ↓
Planning Scene에 Box 추가 ✅
    ↓
Gripper Open / Close ✅
    ↓
Object 위치 조정 ✅
    ↓
Object Attach ✅
```

다음 단계에서는 이 기능들을 연결하여:

```text
Gripper Open
    ↓
Object로 이동
    ↓
Approach
    ↓
Gripper Close
    ↓
Attach
    ↓
Lift
```

순서의 **실제 Pick 동작**을 구현한다.

------------------------------------------------
이렇게 말고 gripper이 box에 직접 가서 attach, pick 시키자 !!
https://github.com/seosleeeep/Physical-AI-Study/blob/main/week03/02_manipulator_pick%26place/Screencast%20from%202026-08-16%2022-01-26.gif
완성본,,,, 아우힘들어


***4. Attach & Detach***

***5. MTC***
  

  
  
