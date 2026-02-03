#include <Arduino.h>
#include <HardwareSerial.h>

// Set to 1 and install EspSoftwareSerial library if you want to use UWB4
#ifndef USE_UWB4_SOFTSERIAL
#define USE_UWB4_SOFTSERIAL 1
#endif

#if USE_UWB4_SOFTSERIAL
#include <SoftwareSerial.h>
#endif

// UART mapping from your wiring table
static constexpr int UWB1_TX = 4;
static constexpr int UWB1_RX = 5;
static constexpr int UWB2_TX = 15;
static constexpr int UWB2_RX = 16;
static constexpr int UWB3_TX = 21;
static constexpr int UWB3_RX = 47;
static constexpr int UWB4_TX = 48;
static constexpr int UWB4_RX = 40;
static constexpr uint32_t UWB_BAUD = 115200;

// Distance line parser settings (edit if your output differs)
static constexpr int DIST_ID_INDEX = 0;     // which numeric token is the ID
static constexpr int DIST_VAL_INDEX = 1;    // which numeric token is the distance

static constexpr uint32_t MEASURE_WINDOW_MS = 15000;
static constexpr uint32_t ROLE_SWITCH_DELAY_MS = 600;
static constexpr uint32_t RESET_DELAY_MS = 1200;
static constexpr uint32_t WAIT_FOR_BOOT_MS = 2500;
static constexpr float MIN_DIST_M = 0.05f;
static constexpr float MAX_DIST_M = 100.0f;
static bool TAG_USE_ID = false;
static bool FORCE_PLANAR = false;

HardwareSerial UWB1(1);  // UART1
HardwareSerial UWB2(2);  // UART2
HardwareSerial UWB3(0);  // UART0 (use USB CDC for log)
#if USE_UWB4_SOFTSERIAL
SoftwareSerial UWB4;
#endif

struct UwbModule {
  const char* name;
  int id;
  Stream* io;
};

static UwbModule modules[] = {
  {"UWB1", 0, &UWB1},
  {"UWB2", 1, &UWB2},
  {"UWB3", 2, &UWB3},
#if USE_UWB4_SOFTSERIAL
  {"UWB4", 3, &UWB4},
#endif
};

static constexpr int kNodeCount = sizeof(modules) / sizeof(modules[0]);

static float distMat[kNodeCount][kNodeCount];
static bool distKnown[kNodeCount][kNodeCount];

static String lineBuf[kNodeCount];
static String inputLine;
static bool rawLog = true;
static String lastCmd[kNodeCount];

static int currentTag = -1;
static uint32_t tagStartAt = 0;
static bool runAuto = false;
static bool autoInited = false;

static void sendAT(Stream& io, const char* name, int idx, const char* cmd) {
  io.print(cmd);
  io.print("\r\n");
  lastCmd[idx] = cmd;
  Serial.print("[CMD ");
  Serial.print(name);
  Serial.print("] ");
  Serial.println(cmd);
}

static void clearMatrix() {
  for (int i = 0; i < kNodeCount; ++i) {
    for (int j = 0; j < kNodeCount; ++j) {
      distMat[i][j] = NAN;
      distKnown[i][j] = false;
    }
  }
}

static void printMatrix() {
  Serial.println("Distance matrix (m):");
  for (int i = 0; i < kNodeCount; ++i) {
    for (int j = 0; j < kNodeCount; ++j) {
      if (i == j) {
        Serial.print("  0.000 ");
      } else if (distKnown[i][j]) {
        Serial.printf("%7.3f ", distMat[i][j]);
      } else {
        Serial.print("  ---- ");
      }
    }
    Serial.println();
  }
}

static bool parseNumbers(const String& s, float* out, int maxCount, int& found) {
  found = 0;
  const char* p = s.c_str();
  while (*p && found < maxCount) {
    if ((*p >= '0' && *p <= '9') || *p == '-' || *p == '.') {
      char* endPtr = nullptr;
      float v = strtof(p, &endPtr);
      if (endPtr != p) {
        out[found++] = v;
        p = endPtr;
        continue;
      }
    }
    ++p;
  }
  return found > 0;
}

static bool isDistanceLine(const String& s) {
  String lower = s;
  lower.toLowerCase();
  if (lower.indexOf("ok") >= 0 || lower.indexOf("error") >= 0) return false;
  if (lower.indexOf("hello") >= 0 || lower.indexOf("init") >= 0) return false;
  if (lower.indexOf("ait-") >= 0 || lower.indexOf("device:") >= 0) return false;
  bool hasKeyword = (lower.indexOf("dis") >= 0) || (lower.indexOf("range") >= 0) ||
                    (lower.indexOf("dist") >= 0) || (lower.indexOf("rng") >= 0);
  bool hasAnchorPrefix = lower.startsWith("an");
  bool startsWithDigit = (lower.length() > 0 && isDigit(lower[0]));
  bool startsWithId = lower.startsWith("id");
  return hasKeyword || hasAnchorPrefix || startsWithDigit || startsWithId;
}

static bool parseDistanceLine(const String& s, int& idOut, float& distOut) {
  if (!isDistanceLine(s)) return false;
  float nums[6];
  int found = 0;
  if (!parseNumbers(s, nums, 6, found)) return false;
  if (found <= max(DIST_ID_INDEX, DIST_VAL_INDEX)) return false;
  idOut = static_cast<int>(nums[DIST_ID_INDEX]);
  distOut = nums[DIST_VAL_INDEX];
  if (idOut < 0 || idOut >= kNodeCount) return false;
  if (distOut < MIN_DIST_M || distOut > MAX_DIST_M) return false;
  return true;
}


static void setAnchor(int index) {
  char buf[32];
  snprintf(buf, sizeof(buf), "AT+anchor_tag=1,%d", modules[index].id);
  sendAT(*modules[index].io, modules[index].name, index, buf);
}

static void setTag(int index) {
  if (TAG_USE_ID) {
    char buf[32];
    snprintf(buf, sizeof(buf), "AT+anchor_tag=0,%d", modules[index].id);
    sendAT(*modules[index].io, modules[index].name, index, buf);
  } else {
    sendAT(*modules[index].io, modules[index].name, index, "AT+anchor_tag=0");
  }
}

static void setInterval(int index, int intervalMs) {
  char buf[32];
  snprintf(buf, sizeof(buf), "AT+interval=%d", intervalMs);
  sendAT(*modules[index].io, modules[index].name, index, buf);
}

static void resetModule(int index) {
  sendAT(*modules[index].io, modules[index].name, index, "AT+RST");
}

static void flushInput(int index) {
  Stream* io = modules[index].io;
  uint32_t start = millis();
  while (millis() - start < 200) {
    while (io->available()) {
      io->read();
    }
    delay(5);
  }
}

static void initAnchors() {
  for (int i = 0; i < kNodeCount; ++i) {
    setAnchor(i);
    delay(ROLE_SWITCH_DELAY_MS);
    resetModule(i);
    delay(WAIT_FOR_BOOT_MS);
    flushInput(i);
    setAnchor(i);
    delay(ROLE_SWITCH_DELAY_MS);
  }
}

static void startRanging(int tagIndex) {
  for (int pass = 0; pass < 2; ++pass) {
    setTag(tagIndex);
    delay(ROLE_SWITCH_DELAY_MS);
    setInterval(tagIndex, 5);
    delay(ROLE_SWITCH_DELAY_MS);
    sendAT(*modules[tagIndex].io, modules[tagIndex].name, tagIndex, "AT+switchdis=1");
    delay(ROLE_SWITCH_DELAY_MS);
    if (pass == 0) {
      resetModule(tagIndex);
      delay(WAIT_FOR_BOOT_MS);
      flushInput(tagIndex);
    }
  }
  tagStartAt = millis();
  currentTag = tagIndex;
}

static void stopRanging(int tagIndex) {
  sendAT(*modules[tagIndex].io, modules[tagIndex].name, tagIndex, "AT+switchdis=0");
  delay(ROLE_SWITCH_DELAY_MS);
  setAnchor(tagIndex);
  delay(ROLE_SWITCH_DELAY_MS);
}

static void printTagSummary(int tagIndex) {
  Serial.print("[TAG ");
  Serial.print(modules[tagIndex].name);
  Serial.println("] distances:");
  bool any = false;
  for (int j = 0; j < kNodeCount; ++j) {
    if (j == tagIndex) continue;
    if (distKnown[tagIndex][j]) {
      any = true;
      Serial.print("  -> ");
      Serial.print(modules[j].name);
      Serial.print(": ");
      Serial.println(distMat[tagIndex][j], 3);
    }
  }
  if (!any) {
    Serial.println("  (no distances)");
  }
}

static bool tagDistancesComplete(int tagIndex) {
  for (int j = 0; j < kNodeCount; ++j) {
    if (j == tagIndex) continue;
    if (!distKnown[tagIndex][j]) return false;
  }
  return true;
}

static void storeDistance(int tagIndex, int anchorId, float d) {
  if (anchorId < 0 || anchorId >= kNodeCount) return;
  if (anchorId == tagIndex) return;
  distMat[tagIndex][anchorId] = d;
  distMat[anchorId][tagIndex] = d;
  distKnown[tagIndex][anchorId] = true;
  distKnown[anchorId][tagIndex] = true;
}

static void processUwbLine(int idx, const String& line) {
  if (rawLog) {
    Serial.print("[RAW ");
    Serial.print(modules[idx].name);
    Serial.print("] ");
    Serial.println(line);
  }
  if (line == "ERROR" || line.endsWith("ERROR")) {
    Serial.print("[ERR ");
    Serial.print(modules[idx].name);
    Serial.print("] last cmd: ");
    Serial.println(lastCmd[idx]);
  }
  if (idx != currentTag) return;
  int id = -1;
  float d = NAN;
  if (parseDistanceLine(line, id, d)) {
    storeDistance(currentTag, id, d);
  }
}

static void readUwbLines(int idx) {
  Stream* io = modules[idx].io;
  while (io->available()) {
    char c = static_cast<char>(io->read());
    if (c == '\n' || c == '\r') {
      if (lineBuf[idx].length() > 0) {
        processUwbLine(idx, lineBuf[idx]);
        lineBuf[idx] = "";
      }
    } else {
      lineBuf[idx] += c;
    }
  }
}

static void solvePositions() {
  if (!distKnown[0][1] || !distKnown[0][2] || !distKnown[1][2] ||
      !distKnown[0][3] || !distKnown[1][3] || !distKnown[2][3]) {
    Serial.println("Not enough distances to solve 3D positions.");
    return;
  }

  float d01 = distMat[0][1];
  float d02 = distMat[0][2];
  float d12 = distMat[1][2];
  float d03 = distMat[0][3];
  float d13 = distMat[1][3];
  float d23 = distMat[2][3];

  if (d01 <= 0.0f) {
    Serial.println("Invalid d01.");
    return;
  }

  float x1 = d01;
  float y1 = 0.0f;
  float z1 = 0.0f;

  float x2 = (d01 * d01 + d02 * d02 - d12 * d12) / (2.0f * d01);
  float y2_sq = d02 * d02 - x2 * x2;
  if (y2_sq < 0.0f) y2_sq = 0.0f;
  float y2 = sqrtf(y2_sq);
  float z2 = 0.0f;

  if (y2 == 0.0f) {
    Serial.println("Degenerate geometry (anchors collinear).");
    return;
  }

  float x3 = (d01 * d01 + d03 * d03 - d13 * d13) / (2.0f * d01);
  float y3 = 0.0f;
  float z3 = 0.0f;

  if (FORCE_PLANAR) {
    float y3_sq = d03 * d03 - x3 * x3;
    if (y3_sq < 0.0f) y3_sq = 0.0f;
    float y3_pos = sqrtf(y3_sq);
    float y3_neg = -y3_pos;
    float err_pos = fabsf((x3 - x2) * (x3 - x2) + (y3_pos - y2) * (y3_pos - y2) - d23 * d23);
    float err_neg = fabsf((x3 - x2) * (x3 - x2) + (y3_neg - y2) * (y3_neg - y2) - d23 * d23);
    y3 = (err_pos <= err_neg) ? y3_pos : y3_neg;
    z3 = 0.0f;
  } else {
    y3 = (d02 * d02 + d03 * d03 - d23 * d23 - 2.0f * x2 * x3) / (2.0f * y2);
    float z3_sq = d03 * d03 - x3 * x3 - y3 * y3;
    if (z3_sq < 0.0f) z3_sq = 0.0f;
    z3 = sqrtf(z3_sq);
  }

  Serial.println("Solved 3D positions (relative):");
  Serial.printf("P0: (0.000, 0.000, 0.000)\n");
  Serial.printf("P1: (%.3f, %.3f, %.3f)\n", x1, y1, z1);
  Serial.printf("P2: (%.3f, %.3f, %.3f)\n", x2, y2, z2);
  Serial.printf("P3: (%.3f, %.3f, %.3f)\n", x3, y3, z3);

  // Residuals (how well distances fit)
  float r01 = fabsf((x1 - 0.0f) * (x1 - 0.0f) - d01 * d01);
  float r02 = fabsf(x2 * x2 + y2 * y2 - d02 * d02);
  float r12 = fabsf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) - d12 * d12);
  float r03 = fabsf(x3 * x3 + y3 * y3 + z3 * z3 - d03 * d03);
  float r13 = fabsf((x3 - x1) * (x3 - x1) + (y3 - y1) * (y3 - y1) + (z3 - z1) * (z3 - z1) - d13 * d13);
  float r23 = fabsf((x3 - x2) * (x3 - x2) + (y3 - y2) * (y3 - y2) + (z3 - z2) * (z3 - z2) - d23 * d23);
  Serial.printf("Residuals (squared error): r01=%.6f r02=%.6f r12=%.6f r03=%.6f r13=%.6f r23=%.6f\n",
                r01, r02, r12, r03, r13, r23);
}

static void startAuto() {
  clearMatrix();
  runAuto = true;
  currentTag = -1;
  autoInited = false;
  Serial.println("[AUTO] start");
}

static void stopAuto() {
  runAuto = false;
  if (currentTag >= 0) stopRanging(currentTag);
  currentTag = -1;
  autoInited = false;
  Serial.println("[AUTO] stop");
}

static void printHelp() {
  Serial.println("Commands:");
  Serial.println("  run       - auto cycle tags and collect distances");
  Serial.println("  stop      - stop auto ranging");
  Serial.println("  matrix    - print distance matrix");
  Serial.println("  solve     - compute 3D positions");
  Serial.println("  clear     - clear matrix");
  Serial.println("  raw on|off- log raw UWB UART lines");
  Serial.println("  tagid on|off - tag uses ID in AT+anchor_tag");
  Serial.println("  planar on|off - force Z=0 solution");
  Serial.println("  m <n> <AT+...> - send AT to module n (1-4)");
  Serial.println("  AT+...    - send raw AT to UWB1");
}

static void handleCommand(String line) {
  line.trim();
  if (line.length() == 0) return;
  String raw = line;
  String lower = line;
  lower.toLowerCase();
  if (lower.startsWith("at+")) {
    sendAT(UWB1, "UWB1", 0, raw.c_str());
    return;
  }
  if (lower == "run") {
    startAuto();
  } else if (lower == "stop") {
    stopAuto();
  } else if (lower == "matrix") {
    printMatrix();
  } else if (lower == "solve") {
    solvePositions();
  } else if (lower == "clear") {
    clearMatrix();
  } else if (lower.startsWith("raw ")) {
    if (lower.endsWith("on")) {
      rawLog = true;
      Serial.println("raw log on");
    } else if (lower.endsWith("off")) {
      rawLog = false;
      Serial.println("raw log off");
    } else {
      Serial.println("Usage: raw on|off");
    }
  } else if (lower.startsWith("tagid ")) {
    if (lower.endsWith("on")) {
      TAG_USE_ID = true;
      Serial.println("tagid on");
    } else if (lower.endsWith("off")) {
      TAG_USE_ID = false;
      Serial.println("tagid off");
    } else {
      Serial.println("Usage: tagid on|off");
    }
  } else if (lower.startsWith("planar ")) {
    if (lower.endsWith("on")) {
      FORCE_PLANAR = true;
      Serial.println("planar on");
    } else if (lower.endsWith("off")) {
      FORCE_PLANAR = false;
      Serial.println("planar off");
    } else {
      Serial.println("Usage: planar on|off");
    }
  } else if (lower.startsWith("m ")) {
    int spacePos = raw.indexOf(' ');
    int nextPos = raw.indexOf(' ', spacePos + 1);
    if (nextPos > 0) {
      int idx = raw.substring(spacePos + 1, nextPos).toInt() - 1;
      if (idx >= 0 && idx < kNodeCount) {
        String cmd = raw.substring(nextPos + 1);
        cmd.trim();
        if (cmd.length() > 0) {
          sendAT(*modules[idx].io, modules[idx].name, idx, cmd.c_str());
        }
      } else {
        Serial.println("Module index must be 1-4.");
      }
    } else {
      Serial.println("Usage: m <n> <AT+...>");
    }
  } else if (lower == "help") {
    printHelp();
  } else {
    Serial.println("Unknown command. Type 'help'.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("UWB auto-ranging + 3D solve (ESP32-S3)");

  UWB1.begin(UWB_BAUD, SERIAL_8N1, UWB1_RX, UWB1_TX);
  UWB2.begin(UWB_BAUD, SERIAL_8N1, UWB2_RX, UWB2_TX);
  UWB3.begin(UWB_BAUD, SERIAL_8N1, UWB3_RX, UWB3_TX);
#if USE_UWB4_SOFTSERIAL
  UWB4.begin(UWB_BAUD, SWSERIAL_8N1, UWB4_RX, UWB4_TX);
#else
  Serial.println("UWB4 disabled (no SoftwareSerial). Set USE_UWB4_SOFTSERIAL=1 to enable.");
#endif

  clearMatrix();
  printHelp();
}

void loop() {
  // Read UWB UARTs
  for (int i = 0; i < kNodeCount; ++i) {
    readUwbLines(i);
  }

  // Read USB serial commands
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (inputLine.length() > 0) {
        handleCommand(inputLine);
        inputLine = "";
      }
    } else {
      inputLine += c;
    }
  }

  if (runAuto) {
    if (!autoInited) {
      initAnchors();
      startRanging(0);
      autoInited = true;
    }

    if (currentTag >= 0) {
      if (tagDistancesComplete(currentTag) ||
          (millis() - tagStartAt) >= MEASURE_WINDOW_MS) {
        stopRanging(currentTag);
        printTagSummary(currentTag);
        currentTag++;
        if (currentTag >= kNodeCount) {
          runAuto = false;
          currentTag = -1;
          Serial.println("[AUTO] done");
          printMatrix();
        } else {
          startRanging(currentTag);
        }
      }
    }
  }
}
