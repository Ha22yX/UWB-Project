#include <Arduino.h>
#include <HardwareSerial.h>
#include <SoftwareSerial.h>  // Install EspSoftwareSerial library if missing

// UART pins mapping (per provided wiring table)
static constexpr int UWB1_TX = 4;
static constexpr int UWB1_RX = 5;
static constexpr int UWB2_TX = 15;
static constexpr int UWB2_RX = 16;
static constexpr int UWB3_TX = 21;
static constexpr int UWB3_RX = 47;
static constexpr int UWB4_TX = 48;
static constexpr int UWB4_RX = 40;

static constexpr uint32_t UWB_BAUD = 115200;

// Use USB CDC for log so UART0 can be used for UWB3.
// In Arduino-ESP32, enable "USB CDC On Boot" to make Serial use USB.
HardwareSerial Uwb3(0);  // UART0 mapped to GPIO21/GPIO47 for UWB3

HardwareSerial& Uwb1 = Serial1;  // UART1 for UWB1
HardwareSerial& Uwb2 = Serial2;  // UART2 for UWB2

// UART4: ESP32-S3 has only 3 hardware UARTs, so use SoftwareSerial for UWB4.
SoftwareSerial Uwb4;

static void sendAT(Stream& port, const char* cmd) {
  port.print(cmd);
  port.print("\r\n");
}

static void bridgeToLog(const char* tag, Stream& port) {
  while (port.available()) {
    int c = port.read();
    Serial.print(tag);
    Serial.write(c);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("UWB UART test start");

  Uwb1.begin(UWB_BAUD, SERIAL_8N1, UWB1_RX, UWB1_TX);
  Uwb2.begin(UWB_BAUD, SERIAL_8N1, UWB2_RX, UWB2_TX);
  Uwb3.begin(UWB_BAUD, SERIAL_8N1, UWB3_RX, UWB3_TX);
  Uwb4.begin(UWB_BAUD, SWSERIAL_8N1, UWB4_RX, UWB4_TX);

  // Example: set anchor mode (ID=0) then reset module.
  sendAT(Uwb1, "AT+anchor_tag=1,0");
  sendAT(Uwb1, "AT+RST");
  sendAT(Uwb2, "AT+anchor_tag=1,0");
  sendAT(Uwb2, "AT+RST");
  sendAT(Uwb3, "AT+anchor_tag=1,0");
  sendAT(Uwb3, "AT+RST");
  sendAT(Uwb4, "AT+anchor_tag=1,0");
  sendAT(Uwb4, "AT+RST");
}

void loop() {
  bridgeToLog("[UWB1] ", Uwb1);
  bridgeToLog("[UWB2] ", Uwb2);
  bridgeToLog("[UWB3] ", Uwb3);
  bridgeToLog("[UWB4] ", Uwb4);
  delay(10);
}

