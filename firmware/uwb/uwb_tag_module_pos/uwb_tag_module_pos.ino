#include <Arduino.h>

// ESP32-S3: UWB tag using module's built-in position output
// UWB1 wiring:
//   ESP32-S3 GPIO4  -> UWB RX (Pin3)
//   ESP32-S3 GPIO5  <- UWB TX (Pin4)
static const int UWB_RX = 5;
static const int UWB_TX = 4;
static const uint32_t BAUD = 115200;

HardwareSerial UWB(1);

void sendAT(const char *cmd) {
  UWB.print(cmd);
  UWB.print("\r\n");
  Serial.print("[CMD] ");
  Serial.println(cmd);
}

String readLineUwb() {
  static String line;
  while (UWB.available()) {
    char c = (char)UWB.read();
    if (c == '\r' || c == '\n') {
      if (line.length() > 0) {
        String out = line;
        line = "";
        return out;
      }
    } else if (c >= 32 && c <= 126) {
      line += c;
      if (line.length() > 160) {
        String out = line;
        line = "";
        return out;
      }
    }
  }
  return "";
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
      if (line.length() > 160) {
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
  Serial.println("  interval <n> - AT+interval=n");
  Serial.println("  at <cmd>     - send raw AT command");
  Serial.println("  help         - show this help");
}

void setup() {
  Serial.begin(115200);
  delay(300);

  UWB.begin(BAUD, SERIAL_8N1, UWB_RX, UWB_TX);

  Serial.println("ESP32-S3 UWB tag (module position output)");
  printHelp();

  // Tag mode, let the module compute and output position info
  sendAT("AT+RST");
  delay(200);
  sendAT("AT+anchor_tag=0");
  delay(100);
  sendAT("AT+interval=5");
  delay(100);
  sendAT("AT+switchdis=1");
}

void loop() {
  // USB commands
  String cmd = readLineUSB();
  if (cmd.length() > 0) {
    cmd.trim();
    if (cmd.startsWith("interval ")) {
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

  // UWB output (position or ranging info from module)
  String line = readLineUwb();
  if (line.length() > 0) {
    Serial.print("[UWB] ");
    Serial.println(line);
  }
}


