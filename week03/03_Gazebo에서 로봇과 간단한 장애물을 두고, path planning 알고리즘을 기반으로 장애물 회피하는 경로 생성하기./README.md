3. Gazebo에서 이전에 만든 모바일 로봇과 간단한 장애물을 두고, path planning 알고리즘을 기반으로 장애물 회피하는 경로 생성하기.

**Nav2**
ROS 2 모바일 로봇 Navigation Framework

**Global Planner**
Start → Goal 전체 경로 생성

**Local Controller**
Global Path를 따라 실제 속도 명령 생성

**Costmap**
장애물과 이동 가능 공간을 비용으로 표현

**LiDAR**
주변 장애물 감지

**/scan**
LiDAR LaserScan Topic

**/odom**
로봇의 Odometry

**TF**
map → odom → base_link 등의 좌표 관계

**Global Path**
Planner가 생성한 전체 이동 경로

**A***
그래프 탐색 기반 경로 계획 알고리즘

**Smac Planner**
Nav2에서 사용 가능한 A* 기반 Planner

------------------------------------------------------------------
## pipeline     
STEP 1  간단한 Differential Drive 로봇 생성
        └─ base_link + 좌/우 wheel 구조 만들기
        ↓
STEP 2  Gazebo에서 로봇 띄우기
        └─ URDF/Xacro → Gazebo spawn 확인
        ↓
STEP 3  바퀴 구동 + /cmd_vel
        └─ linear.x / angular.z 명령으로 실제 이동 확인
        ↓
STEP 4  Odometry / TF 구성
        └─ odom → base_link 위치 관계 확인
        ↓
STEP 5  LiDAR 추가
        └─ /scan으로 장애물 거리 측정
        ↓
STEP 6  Gazebo 장애물 추가
        └─ Box 등을 world에 배치
        ↓
STEP 7  Nav2 연결
        └─ Planner / Controller / Costmap 구성
        ↓
STEP 8  RViz에서 Goal 지정
        └─ 목표 위치와 방향 지정
        ↓
STEP 9  장애물을 피하는 Global Path 생성
        └─ Costmap 기반 충돌 없는 경로 계산
        ↓
STEP 10 실제 로봇이 경로 따라 이동
        └─ Controller → /cmd_vel → Differential Drive  
------------------------------------------------------------------
## step1> 간단한 Differential Drive 로봇 생성

### STEP 1 결과

★ `URDF/Xacro`를 이용하여 Differential Drive 모바일 로봇 모델 생성

★ `base_link`를 기준으로 좌/우 Wheel 및 Caster 구성

★ 좌/우 Wheel Joint를 `continuous` 타입으로 설정

★ Xacro → URDF 변환 및 문법 검사 성공

```bash
xacro src/mobile_robot_description/urdf/mobile_robot.urdf.xacro \
  > /tmp/mobile_robot.urdf

check_urdf /tmp/mobile_robot.urdf

```
//실행결과 
rrc@rrc-15Z90R-GA5UK:~/nav2_ws$ check_urdf /tmp/mobile_robot.urdf
robot name is: mobile_robot
---------- Successfully Parsed XML ---------------
root Link: base_link has 3 child(ren)
    child(1):  caster_link
    child(2):  left_wheel_link
    child(3):  right_wheel_link
rrc@rrc-15Z90R-GA5UK:~/nav2_ws$ xacro src/mobile_robot_description/urdf/mobile_robot.urdf.xacro \
  > /tmp/mobile_robot.urdf

check_urdf /tmp/mobile_robot.urdf
robot name is: mobile_robot
---------- Successfully Parsed XML ---------------
root Link: base_link has 3 child(ren)
    child(1):  caster_link
    child(2):  left_wheel_link
    child(3):  right_wheel_link
```






