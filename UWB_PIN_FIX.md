# ESP32-S3 UWB 针脚问题修复

## 问题原因

**GPIO15 和 GPIO16 是 ESP32-S3 的 Strapping 引脚**
- 在启动期间用于选择 boot 模式
- 内置 4KΩ 上拉电阻
- UART2 在这些引脚上可能无法正常工作

## 新的针脚配置

| UWB 模块 | 原配置 | 修复后配置 | 状态 |
|----------|--------|------------|------|
| UWB1 | TX=GPIO4, RX=GPIO5 | TX=GPIO4, RX=GPIO5 | ✓ 正常 |
| UWB2 | TX=GPIO15, RX=GPIO16 | **TX=GPIO17, RX=GPIO16** | ✓ 已修复 |
| UWB3 | TX=GPIO21, RX=GPIO47 | TX=GPIO21, RX=GPIO47 | ✓ 正常 |

## 接线表

| UWB 模块 | ESP32 TX | ESP32 RX | 说明 |
|----------|----------|----------|------|
| UWB1 | GPIO4 | GPIO5 | 正常配置 |
| **UWB2** | **GPIO17** | **GPIO16** | 修复：避免 GPIO15 |
| UWB3 | GPIO21 | GPIO47 | 正常配置 |

## 修复说明

1. **将 UWB2 TX 从 GPIO15 改为 GPIO17**
   - GPIO17 是普通 GPIO，不受 strapping 限制
   - 保留 GPIO16 作为 RX（但需要小心处理）

2. **代码修复**
   - 增加了启动延迟，确保 boot 完成后再初始化 UART2
   - 添加了错误检测和响应处理

3. **兼容性**
   - 新配置使用标准 UART 引脚
   - 不影响其他功能
   - 所有 GPIO 都可用作 UART

## 测试建议

1. 先使用 `sketch_diagnosis.ino` 测试每个模块是否响应
2. 然后使用 `sketch_fixed_pins.ino` 进行完整的测距测试
3. 如果还有问题，检查：
   - 电源供应（3.3V 稳定）
   - 接线牢固度
   - 模块是否正常工作

## 额外优化

- 改进了 AT 命令发送机制
- 添加了超时检测
- 优化了错误信息显示
- 简化了用户命令系统
