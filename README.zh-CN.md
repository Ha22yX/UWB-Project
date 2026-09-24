<div align="center">
  <h1>UWB Project</h1>
  <p>面向母机对接无人机系统的 UWB 测距、三边定位、ESP32-S3 固件和可视化实验仓库。</p>

  <p>
    <a href="README.md">English</a>
    &middot;
    <a href="https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System">主项目</a>
    &middot;
    <a href="https://github.com/Ha22yX/OpenMV-AprilTag">视觉模块</a>
    &middot;
    <a href="https://isef.rosebeg.com">项目网站</a>
    &middot;
    <a href="#快速开始">快速开始</a>
    &middot;
    <a href="#核心能力">核心能力</a>
    &middot;
    <a href="#技术栈">技术栈</a>
  </p>

  <p>
    <img alt="Arduino: ESP32-S3" src="https://img.shields.io/badge/Arduino-ESP32--S3-00878F?style=for-the-badge&logo=arduino&logoColor=white" />
    <img alt="UWB: trilateration" src="https://img.shields.io/badge/UWB-trilateration-287866?style=for-the-badge" />
    <img alt="Pixhawk: MAVLink" src="https://img.shields.io/badge/Pixhawk-MAVLink-2f6f67?style=for-the-badge" />
    <img alt="Status: bench tests" src="https://img.shields.io/badge/Status-bench%20tests-6b7f73?style=for-the-badge" />
  </p>
</div>

<p align="center">
  <img src=".github/assets/readme-hero.svg" alt="UWB Project 项目概览图" width="100%" />
</p>

## 项目概览

这个仓库负责 Mother-Ship Docking Drone System 中的中距离相对定位层。GPS/RTK 可以让子无人机接近母机平台，但真正对接时需要母机坐标系下的局部位置估计。

本仓库的重点是把 UWB 锚点到标签的距离转换成可观察的 `x, y, z` 估计。UWB 模块本身保留在这里，把定位结果接入飞控的程序放在主项目中。

## 无人机固件已迁入主项目

开发 UWB 模块时，我把一些母机和子机控制程序也放进了这个仓库。现在将这些文件整理到了 [Mother-Ship-Docking-Drone-System](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System)，方便在主项目中一起查找整机实验。

| 在本仓库中的原位置 | 在主项目中的新位置 |
| --- | --- |
| `firmware/docking/` | [母机/子机 ESP-NOW 固件](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/tree/main/firmware/docking) |
| `firmware/pixhawk/` | [Pixhawk 通信与控制实验](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/tree/main/firmware/pixhawk) |
| `firmware/openmv/` | [ESP32 端 OpenMV 接入实验](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/tree/main/firmware/openmv) |
| `firmware/uwb/uwb_follow_pixhawk/` | [UWB 接入 Pixhawk 的跟随实验](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/tree/main/firmware/integration/uwb_follow_pixhawk) |
| `archive/old-main/` | [早期整机原型](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/tree/main/archive/old-main) |
| `tools/debug/sik_debug.py` | [SiK 数传调试工具](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/blob/main/tools/debug/sik_debug.py) |
| `tools/visualization/world_camera.py` | [AprilTag 位姿查看器](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/blob/main/tools/visualization/world_camera.py) |
| `docs/reference/px4_mavlink_docs.md` | [PX4 / MAVLink 参考资料](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/blob/main/docs/reference/px4_mavlink_docs.md) |

[迁移说明](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/blob/main/docs/repository-layout.md)记录了原始提交和迁移的 55 个文件。UWB 测距、锚点/标签固件、独立位置解算、UWB 可视化、接线说明和模块资料仍保留在这里。迁移保留了代码原始内容。

## 核心能力

- ESP32-S3 Arduino 草图，覆盖 UWB 锚点/标签通信和测距。
- 面向母机对接框架的四锚点相对定位几何。
- 通过三边定位/最小二乘把距离转换成局部位置。
- Python 串口查看器，用于检查 UWB 距离和位置数据。
- 作为主对接项目的 UWB 附属模块，与 OpenMV AprilTag 视觉模块互补。

## 工作方式

1. 在母机对接框架上布置 UWB 锚点，在子无人机上布置 UWB 标签。
2. 通过 ESP32-S3/UWB 硬件链路读取各锚点距离。
3. 对距离跳变进行过滤，并映射到已配置的锚点几何。
4. 解算相对位置，并通过 PC 工具可视化。
5. 在末端视觉接管前，把结果作为中距离定位信号。

## 快速开始

克隆仓库，安装 PC 可视化依赖，然后在 `firmware/uwb/` 中选择需要的草图，用 Arduino IDE 或 ESP32 工具链打开。

```bash
git clone https://github.com/Ha22yX/UWB-Project.git
cd UWB-Project
pip install -r tools/requirements.txt
# 在 Arduino IDE 中打开 firmware/uwb/ 下与目录同名的 .ino
python tools/visualization/uwb_viewer.py
```

运行前请根据实际硬件修改串口、UWB ID、锚点坐标和波特率。`uwb_viewer.py` 还需要 Python 环境支持 Tkinter。涉及飞控的草图请查看[主项目固件说明](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System/blob/main/firmware/README.md)。

## 配置项

| 项目 | 需要调整的内容 |
| --- | --- |
| 锚点几何 | 测量母机对接框上的锚点位置，并保持单位一致。 |
| 串口 | 设置查看器使用的 ESP32-S3 / UWB 串口和波特率。 |
| UWB ID | 让固件中的锚点/标签 ID 与物理模块一致。 |

## 技术栈

| 层级 | 技术 | 作用 |
| --- | --- | --- |
| 固件 | Arduino, ESP32-S3 | UWB 锚点/标签与测距实验。 |
| 定位 | UWB 三边定位 | 把距离转换成母机坐标系下的相对位置。 |
| 可视化 | pyserial, matplotlib, Tkinter | 检查 UWB 串口、距离和位置数据。 |
| 主项目中的接入实验 | Pixhawk, MAVLink | 将位置估计接入飞控实验。 |

## 项目结构

```text
firmware/uwb/             UWB 测距和解算草图
tools/visualization/      UWB 距离与位置查看器
tools/requirements.txt    UWB 查看器依赖
docs/                    UWB 接线说明和模块参考资料
archive/prototypes/      早期 UWB 原型
```

## 项目状态

这是硬件研究工作区，适合台架测试和文档沉淀；它不是可直接实飞的飞控软件包，也不是安全认证过的对接控制器。

## 相关项目

- [Mother-Ship-Docking-Drone-System](https://github.com/Ha22yX/Mother-Ship-Docking-Drone-System) - 主无人机对接项目。
- [OpenMV-AprilTag](https://github.com/Ha22yX/OpenMV-AprilTag) - 末端视觉定位模块。

## 许可证

[MIT](LICENSE)。
