# UWB-Project

[English](README.md)

UWB-Project 是一个围绕 UWB 模块位置计算和无人机集成实验整理的项目。它的核心目标是通过 UWB 模块计算相对位置，并将这个位置结果用于子母无人机空中对接系统中的近距离定位与辅助控制。

这个仓库包含 ESP32-S3/Arduino 固件、Python 可视化与串口调试工具、接线说明、模块资料，以及早期实验版本。它更像是一个定位算法与飞控集成实验工作区，而不是一个单一的软件库。

## 项目目标

- 读取 ESP32-S3 连接的 UWB 模块测距输出。
- 基于固定 anchor 布局估算 tag 的相对位置。
- 在桌面端实时可视化 UWB 位置和串口输出，方便调试。
- 将 UWB 定位实验与 Pixhawk/MAVLink 遥测和控制链路连接起来。
- 通过 ESP-NOW 支持子母无人机节点之间的通信。
- 保留旧版实验代码，但把它们和当前主线代码分开。

## 项目结构

```text
firmware/
  docking/      当前子母无人机对接主固件，使用 ESP-NOW 和 MAVLink。
  uwb/          UWB 模块测试、anchor/tag 程序、定位求解和跟随实验。
  pixhawk/      Pixhawk TELEM1 嗅探、解析和控制测试程序。
  openmv/       OpenMV UART 和 AprilTag Web 显示实验。

tools/
  visualization/  UWB 和相机位姿串口输出的 Python 可视化工具。
  debug/          串口、MAVLink、SiK 电台调试脚本。

docs/
  hardware/    UWB 接线和 ESP32-S3 引脚修正说明。
  reference/   PX4/MAVLink 参考笔记。

vendor/
  datasheets/  UWB 模块 PDF、AT 指令和资料。

archive/
  old-main/    早期子母无人机对接实现和辅助脚本。
  prototypes/  保留用于参考的一次性实验 sketch。
```

## 主要固件

### 子母无人机空中对接

- `firmware/docking/UAVDocking_Mother_ESPNOW`
- `firmware/docking/UAVDocking_Child_ESPNOW`

这两个目录是当前主线的子母无人机对接固件。它们使用 ESP-NOW 进行节点间通信，并通过 Pixhawk 遥测口使用 MAVLink。母机端固件还包含 Web Server 界面。

### UWB 位置计算

主要 UWB sketch 位于 `firmware/uwb/`：

- `uwb_anchors_esp32_1`：ESP32-S3 anchor 侧多 UWB 模块配置。
- `uwb_tag_esp32_2`：tag 侧定位求解，包含 anchor 几何和滤波逻辑。
- `uwb_tag_solver`：tag 侧位置求解实验。
- `uwb_tag_module_pos`：读取模块内置位置输出。
- `uwb_follow_pixhawk`：将 UWB tag 位置与 Pixhawk MAVLink 控制实验结合。
- `uwb_single_test`、`uwb3_test`、`uwb_two_nodes`、`uwb_esp32s3_uart_test`：串口、接线、角色切换和基础通信测试。

多个 sketch 当前使用的 anchor 几何是边长 35.5 英寸的正方形，四个 anchor 位于四条边的中点。如果实际安装布局不同，需要先修改对应 sketch 中的几何常量。

## 工具脚本

安装 Python 依赖：

```bash
pip install -r tools/requirements.txt
```

常用脚本：

- `tools/visualization/uwb_viewer.py`：基于 Tkinter/Matplotlib 的 UWB 3D 位置可视化工具。
- `tools/visualization/uwb_tag_viewer.py`：显示 anchor/tag 距离和 tag 位置的 Matplotlib 工具。
- `tools/visualization/world_camera.py`：用于 OpenMV AprilTag 输出的 pyqtgraph 3D 相机位姿查看器。
- `tools/debug/sik_debug.py`：SiK 数传电台和 MAVLink heartbeat 检测的串口调试脚本。

多数脚本在文件顶部有 `PORT` 和 `BAUD` 常量。运行前根据本机串口号和波特率修改即可。

## 硬件说明

参考文件：

- `docs/hardware/uwb_wiring.md`
- `docs/hardware/UWB_PIN_FIX.md`
- `vendor/datasheets/m5_uwb.txt`
- `vendor/datasheets/bu01db_at.txt`

多个 sketch 使用的 ESP32-S3/UWB 接线：

```text
UWB1: ESP32-S3 GPIO4  -> UWB RX, GPIO5  <- UWB TX
UWB2: ESP32-S3 GPIO15 -> UWB RX, GPIO16 <- UWB TX
UWB3: ESP32-S3 GPIO21 -> UWB RX, GPIO47 <- UWB TX
UWB4: ESP32-S3 GPIO48 -> UWB RX, GPIO40 <- UWB TX
```

Pixhawk TELEM 测试接线：

```text
Pixhawk TELEM TX -> ESP32-S3 RX GPIO10
Pixhawk TELEM RX -> ESP32-S3 TX GPIO11
GND              -> GND
```

## 开发环境

Arduino sketch 主要面向 ESP32-S3。根据具体 sketch，可能需要安装：

- Arduino ESP32 开发板支持。
- Pixhawk 相关 sketch 需要 MAVLink Arduino 头文件或库。
- 启用第四路 UWB 串口的 sketch 需要 EspSoftwareSerial。

从每个 sketch 所在目录打开，例如：

```text
firmware/uwb/uwb_tag_solver/uwb_tag_solver.ino
firmware/docking/UAVDocking_Mother_ESPNOW/UAVDocking_Mother_ESPNOW.ino
```

Arduino IDE 要求 `.ino` 文件名和目录名一致；当前结构保留了这个规则。

## 归档目录

`archive/old-main/` 保存早期基于 Wi-Fi/HTTP 的子母无人机对接实现和辅助脚本。`archive/prototypes/` 保存仍有参考价值、但不属于当前主线的一次性实验 sketch。

## License

当前仓库暂未包含 license 文件。
