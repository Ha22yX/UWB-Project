#include <Arduino.h>
#include <HardwareSerial.h>

// UWB1 wiring
static constexpr int UWB1_TX = 4;
static constexpr int UWB1_RX = 5;
// UWB2 wiring
static constexpr int UWB2_TX = 15;
static constexpr int UWB2_RX = 16;

static constexpr uint32_t UWB_BAUD = 115200;
static constexpr uint32_t ROLE_SWITCH_DELAY_MS = 600;
static constexpr uint32_t WAIT_FOR_BOOT_MS = 2500;

HardwareSerial UWB1(1);  // UART1
HardwareSerial UWB2(2);  // UART2

static String lineBuf1;
static String lineBuf2;

static void sendAT(Stream& io, const char* cmd) {
  io.print(cmd);
  io.print("\r\n");
}

static void setupAnchor() {
  // Anchor: UWB1 -> ID0
  sendAT(UWB1, "AT+anchor_tag=1,0");
  delay(ROLE_SWITCH_DELAY_MS);
  sendAT(UWB1, "AT+RST");
  delay(WAIT_FOR_BOOT_MS);
  sendAT(UWB1, "AT+anchor_tag=1,0");
  delay(ROLE_SWITCH_DELAY_MS);
}

static void setupTag() {
  // Tag: UWB2
  sendAT(UWB2, "AT+anchor_tag=0");
  delay(ROLE_SWITCH_DELAY_MS);
  sendAT(UWB2, "AT+interval=5");
  delay(ROLE_SWITCH_DELAY_MS);
  sendAT(UWB2, "AT+switchdis=1");
  delay(ROLE_SWITCH_DELAY_MS);
  // Reset once as in M5Stack example
  sendAT(UWB2, "AT+RST");
  delay(WAIT_FOR_BOOT_MS);
  sendAT(UWB2, "AT+anchor_tag=0");
  delay(ROLE_SWITCH_DELAY_MS);
  sendAT(UWB2, "AT+interval=5");
  delay(ROLE_SWITCH_DELAY_MS);
  sendAT(UWB2, "AT+switchdis=1");
  delay(ROLE_SWITCH_DELAY_MS);
}

static bool parseDistance(const String& line, int& anchorId, float& out) {
  // Expect: an0:0.57m
  if (!line.startsWith("an")) return false;
  int colon = line.indexOf(':');
  if (colon < 0 || colon > 4) return false;
  anchorId = line.substring(2, colon).toInt();
  int mpos = line.indexOf('m', colon + 1);
  if (mpos < 0) return false;
  String val = line.substring(colon + 1, mpos);
  val.trim();
  out = val.toFloat();
  return out > 0.0f;
}

static void readUwb(Stream& io, String& buf, const char* tag) {
  while (io.available()) {
    char c = static_cast<char>(io.read());
    if (c == '\n' || c == '\r') {
      if (buf.length() > 0) {
        int anchorId = -1;
        float dist = 0.0f;
        if (parseDistance(buf, anchorId, dist)) {
          if (anchorId == 0) {  // Only keep distance to UWB1 (anchor ID 0)
            Serial.print("[DIST] ");
            Serial.print(tag);
            Serial.print(": ");
            Serial.print(dist, 3);
            Serial.println(" m");
          }
        }
        buf = "";
      }
    } else {
      if (c >= 32 && c <= 126) {
        buf += c;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("UWB1-UWB2 ranging start");

  UWB1.begin(UWB_BAUD, SERIAL_8N1, UWB1_RX, UWB1_TX);
  UWB2.begin(UWB_BAUD, SERIAL_8N1, UWB2_RX, UWB2_TX);

  setupAnchor();
  setupTag();
}

void loop() {
  readUwb(UWB2, lineBuf2, "UWB1");
  delay(10);
}

