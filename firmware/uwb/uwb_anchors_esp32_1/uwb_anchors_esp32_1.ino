#include <Arduino.h>
#include <SoftwareSerial.h>  // EspSoftwareSerial

// ESP32 #1: 4x UWB anchors
// UWB1: TX=GPIO4  RX=GPIO5
// UWB2: TX=GPIO15 RX=GPIO16
// UWB3: TX=GPIO21 RX=GPIO47
// UWB4: TX=GPIO48 RX=GPIO40

HardwareSerial UWB1(1);
HardwareSerial UWB2(0);
HardwareSerial UWB3(2);
SoftwareSerial UWB4;
static const bool ENABLE_UWB4 = false;

static const uint32_t BAUD = 115200;

struct UwbPort {
  HardwareSerial *serial;
  const char *name;
  int rx;
  int tx;
  int id;
  String buf;
};

UwbPort ports[] = {
    {&UWB1, "UWB1", 5, 4, 0, ""},
    {&UWB2, "UWB2", 16, 15, 1, ""},
    {&UWB3, "UWB3", 47, 21, 2, ""},
};

// UWB4 SoftwareSerial pins
static const int UWB4_RX = 40;
static const int UWB4_TX = 48;

void sendAT(UwbPort &p, const char *cmd) {
  p.serial->print(cmd);
  p.serial->print("\r\n");
  Serial.print("[");
  Serial.print(p.name);
  Serial.print("] ");
  Serial.println(cmd);
}

void sendAT4(const char *cmd) {
  UWB4.print(cmd);
  UWB4.print("\r\n");
  Serial.print("[UWB4] ");
  Serial.println(cmd);
}

String readLine(UwbPort &p) {
  while (p.serial->available()) {
    char c = (char)p.serial->read();
    if (c == '\r' || c == '\n') {
      if (p.buf.length() > 0) {
        String out = p.buf;
        p.buf = "";
        return out;
      }
    } else if (c >= 32 && c <= 126) {
      p.buf += c;
      if (p.buf.length() > 120) {
        String out = p.buf;
        p.buf = "";
        return out;
      }
    }
  }
  return "";
}

String readLineSoft(SoftwareSerial &s) {
  static String line;
  while (s.available()) {
    char c = (char)s.read();
    if (c == '\r' || c == '\n') {
      if (line.length() > 0) {
        String out = line;
        line = "";
        return out;
      }
    } else if (c >= 32 && c <= 126) {
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

void flushFor(uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) {
    for (auto &p : ports) {
      String line = readLine(p);
      if (line.length() > 0) {
        Serial.print("[");
        Serial.print(p.name);
        Serial.print("] ");
        Serial.println(line);
      }
    }
    if (ENABLE_UWB4) {
      String l4 = readLineSoft(UWB4);
      if (l4.length() > 0) {
        Serial.print("[UWB4] ");
        Serial.println(l4);
      }
    }
    delay(5);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  for (auto &p : ports) {
    p.serial->begin(BAUD, SERIAL_8N1, p.rx, p.tx);
  }
  if (ENABLE_UWB4) {
    UWB4.begin(BAUD, SWSERIAL_8N1, UWB4_RX, UWB4_TX);
  }

  Serial.println("ESP32 #1 UWB anchors init");

  // Set anchors with fixed IDs
  for (auto &p : ports) {
    String cmd = "AT+anchor_tag=1,";
    cmd += p.id;
    sendAT(p, cmd.c_str());
    delay(100);
    sendAT(p, "AT+RST");
    delay(300);
    sendAT(p, cmd.c_str());
    delay(100);
  }

  // Configure UWB4 as anchor ID 3
  if (ENABLE_UWB4) {
    sendAT4("AT+anchor_tag=1,3");
    delay(100);
    sendAT4("AT+RST");
    delay(300);
    sendAT4("AT+anchor_tag=1,3");
    delay(100);
  }

  Serial.println("Anchors configured.");

  // Print responses for a short window after init
  flushFor(2000);
}

void loop() {
  for (auto &p : ports) {
    String line = readLine(p);
    if (line.length() > 0) {
      Serial.print("[");
      Serial.print(p.name);
      Serial.print("] ");
      Serial.println(line);
    }
  }
  if (ENABLE_UWB4) {
    String l4 = readLineSoft(UWB4);
    if (l4.length() > 0) {
      Serial.print("[UWB4] ");
      Serial.println(l4);
    }
  }
}

