# 매니퓰레이터를 불러와서 Moveit or Moveit2 (for ROS2) tutorial 을 따라하면서, pick and place 구현하기

## MoveIt2 
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
  
  *1. MotionPlanning 패널 확인* 
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

  *2. Planning Group 확인*
  planning group : MoveIt이 '어떤 joint 묶음을 같이 움직일 것인가'를 정의한 것.\
   panda_arm 선택 -> Panda의 7개 arm joint를 하나의 그룹으로 움직임.

  *3. End Effector의 Interactive Marker 움직이기*
  : Goal State를 수정해보기, IK

| 원래 state | IK solve success |
| --- | --- |
| <img src="https://github.com/user-attachments/assets/e30b65a2-ea3a-4926-b6c6-5be89bfacb2f" width="100%" /> | <img src="https://github.com/user-attachments/assets/8abc6e6c-0178-4f8e-ac6b-bd8fb5b8c784" width="100%" /> |

```
  MotionPlanning>Plan
   Current State -> Goal State -> Motion Planner -> 충돌 검사 -> Trajectory 생성

```



  

  
  
