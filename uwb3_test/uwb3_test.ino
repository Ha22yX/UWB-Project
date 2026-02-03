#include <Arduino.h>

// UWB3 wiring
// ESP32-S3 GPIO21 -> UWB RX (Pin3)
// ESP32-S3 GPIO47 <- UWB TX (Pin4)
static const int UWB_RX = 47;
static const int UWB_TX = 21;

HardwareSerial UWB(2);
static const uint32_t BAUD = 115200;

void sendAT(const char *cmd) {
  UWB.print(cmd);
  UWB.print("\r\n");
  Serial.print("[CMD] ");
  Serial.println(cmd);
}

String readLineUSB() {
  static String line;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      if (line.length() > 0) {
        String out = line;
        line = "";
        return out;
      }
    } else {
      line += c;
      if (line.length() > 120) {
        String out = line;
        line = "";
        return out;
      }
    }
  }
  return "";
}

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  ver        - AT+version?");
  Serial.println("  rst        - AT+RST");
  Serial.println("  anchor     - AT+anchor_tag=1,2");
  Serial.println("  tag        - AT+anchor_tag=0");
  Serial.println("  dist on    - AT+switchdis=1");
  Serial.println("  dist off   - AT+switchdis=0");
  Serial.println("  interval n - AT+interval=n");
  Serial.println("  at <cmd>   - send raw AT command");
}

void setup() {
  Serial.begin(115200);
  delay(300);

  UWB.begin(BAUD, SERIAL_8N1, UWB_RX, UWB_TX);

  Serial.println("ESP32-S3 UWB3 test");
  printHelp();
}

void loop() {
  // USB -> UWB
  String cmd = readLineUSB();
  if (cmd.length() > 0) {
    cmd.trim();
    if (cmd == "ver") {
      sendAT("AT+version?");
    } else if (cmd == "rst") {
      sendAT("AT+RST");
    } else if (cmd == "anchor") {
      sendAT("AT+anchor_tag=1,2");
    } else if (cmd == "tag") {
      sendAT("AT+anchor_tag=0");
    } else if (cmd == "dist on") {
      sendAT("AT+switchdis=1");
    } else if (cmd == "dist off") {
      sendAT("AT+switchdis=0");
    } else if (cmd.startsWith("interval ")) {
      String n = cmd.substring(9);
      n.trim();
      String at = "AT+interval=" + n;
      sendAT(at.c_str());
    } else if (cmd.startsWith("at ")) {
      String at = cmd.substring(3);
      at.trim();
      sendAT(at.c_str());
    } else if (cmd == "help") {
      printHelp();
    } else {
      Serial.println("Unknown command. Type 'help'.");
    }
  }

  // UWB -> USB
  while (UWB.available()) {
    char c = (char)UWB.read();
    Serial.write(c);
  }
}

