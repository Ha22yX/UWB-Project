## ESP32-S3 + M5Stack UWB (UART) 接线对应表

### 统一接线规则（4 个 UWB 模块一致）

| ZX-HY2.0 4Pin | 说明 | 连接到 ESP32-S3 |
| --- | --- | --- |
| Pin1 | VCC（建议 3V3） | 3V3 |
| Pin2 | GND | GND |
| Pin3 | UWB_RX（模块接收） | ESP32 TX |
| Pin4 | UWB_TX（模块发送） | ESP32 RX |

### 4 组 TX/RX 分配（基站端 4 个 UWB）

| UWB 模块 | ESP32 TX | ESP32 RX | ZX Pin3 (UWB_RX) | ZX Pin4 (UWB_TX) |
| --- | --- | --- | --- | --- |
| UWB1 | GPIO4 | GPIO5 | ← GPIO4 | → GPIO5 |
| UWB2 | GPIO15 | GPIO16 | ← GPIO15 | → GPIO16 |
| UWB3 | GPIO21 | GPIO47 | ← GPIO21 | → GPIO47 |
| UWB4 | GPIO48 | GPIO40 | ← GPIO48 | → GPIO40 |

### 参考文档

- M5Stack UWB Unit 文档（中文）：https://docs.m5stack.com/zh_CN/unit/uwb
- M5Stack UWB Unit 手册 PDF：https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/zh_CN/unit/uwb.pdf
- ESP32-S3 数据手册：https://documentation.espressif.com/esp32-s3_datasheet_cn.html

