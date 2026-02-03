#include <Arduino.h>

// ESP32 #2: 1x UWB tag
// UWB1: TX=GPIO4  RX=GPIO5
static const int UWB_RX = 5;
static const int UWB_TX = 4;

HardwareSerial UWB(1);
static const uint32_t BAUD = 115200;

// Square side length: 35.5 inches
static const float INCH_TO_M = 0.0254f;
static const float SIDE_M = 35.5f * INCH_TO_M;
static const float HALF = SIDE_M * 0.5f;
static const float AZ[4] = {0.0f, 0.0f, 0.0f, 0.0f};

// Anchor coordinates (meters), midpoints of 4 edges
// Physical order clockwise: 1 -> 2 -> 3 -> 4
// Mapping to UWB IDs (an0..an3):
//   an0 = Anchor 1 (right midpoint)
//   an1 = Anchor 2 (bottom midpoint)
//   an2 = Anchor 3 (left midpoint)
//   an3 = Anchor 4 (top midpoint)
static const float AX[4] = {+HALF, 0.0f, -HALF, 0.0f};
static const float AY[4] = {0.0f, -HALF, 0.0f, +HALF};

float distM[4] = {-1, -1, -1, -1};
static const int FILTER_N = 5;
float distHist[4][FILTER_N] = {{0}};
int distCount[4] = {0, 0, 0, 0};
int distIndex[4] = {0, 0, 0, 0};

float median5(float *v, int n) {
  float a[5];
  for (int i = 0; i < n; ++i) a[i] = v[i];
  for (int i = 0; i < n; ++i) {
    for (int j = i + 1; j < n; ++j) {
      if (a[j] < a[i]) {
        float t = a[i];
        a[i] = a[j];
        a[j] = t;
      }
    }
  }
  return a[n / 2];
}

float filterDistance(int id, float v) {
  distHist[id][distIndex[id]] = v;
  distIndex[id] = (distIndex[id] + 1) % FILTER_N;
  if (distCount[id] < FILTER_N) distCount[id]++;
  return median5(distHist[id], distCount[id]);
}

void sendAT(const char *cmd) {
  UWB.print(cmd);
  UWB.print("\r\n");
  Serial.print("[CMD] ");
  Serial.println(cmd);
}

bool parseDistanceLine(const String &line, int &id, float &m) {
  // Expected: an0:0.57m
  if (!line.startsWith("an")) return false;
  int colon = line.indexOf(':');
  int mpos = line.indexOf('m');
  if (colon < 0 || mpos < 0) return false;
  String idStr = line.substring(2, colon);
  String valStr = line.substring(colon + 1, mpos);
  id = idStr.toInt();
  m = valStr.toFloat();
  if (id < 0 || id > 3) return false;
  return true;
}

bool solve2D(float &x, float &y) {
  if (distM[0] <= 0 || distM[1] <= 0 || distM[2] <= 0 || distM[3] <= 0)
    return false;

  // Linearized least squares using anchor 0 as reference
  // 2(xi-x0)x + 2(yi-y0)y = (xi^2+yi^2-di^2) - (x0^2+y0^2-d0^2)
  float x0 = AX[0], y0 = AY[0], d0 = distM[0];
  float A11 = 0, A12 = 0, A22 = 0;
  float B1 = 0, B2 = 0;
  for (int i = 1; i < 4; ++i) {
    float xi = AX[i], yi = AY[i], di = distM[i];
    float ai = 2.0f * (xi - x0);
    float bi = 2.0f * (yi - y0);
    float ci = (xi * xi + yi * yi - di * di) -
               (x0 * x0 + y0 * y0 - d0 * d0);
    A11 += ai * ai;
    A12 += ai * bi;
    A22 += bi * bi;
    B1 += ai * ci;
    B2 += bi * ci;
  }

  float det = A11 * A22 - A12 * A12;
  if (fabs(det) < 1e-6f) return false;

  x = (B1 * A22 - B2 * A12) / det;
  y = (A11 * B2 - A12 * B1) / det;
  return true;
}

bool solve3DWithMirror(float &x, float &y, float &z) {
  if (!solve2D(x, y)) return false;
  float zsum = 0.0f;
  int zcount = 0;
  for (int i = 0; i < 4; ++i) {
    float dx = x - AX[i];
    float dy = y - AY[i];
    float z2 = distM[i] * distM[i] - dx * dx - dy * dy;
    if (z2 > 0) {
      zsum += sqrtf(z2);
      zcount++;
    }
  }
  if (zcount == 0) return false;
  z = zsum / zcount; // positive mirror only
  return true;
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
      if (line.length() > 120) {
        String out = line;
        line = "";
        return out;
      }
    }
  }
  return "";
}

void setup() {
  Serial.begin(115200);
  delay(300);
  UWB.begin(BAUD, SERIAL_8N1, UWB_RX, UWB_TX);

  Serial.println("ESP32 #2 UWB tag + 2D solve");

  sendAT("AT+anchor_tag=0");
  delay(100);
  sendAT("AT+interval=10");
  delay(100);
  sendAT("AT+switchdis=1");
}

void loop() {
  String line = readLineUwb();
  if (line.length() > 0) {
    int id;
    float m;
    if (parseDistanceLine(line, id, m)) {
      distM[id] = filterDistance(id, m);
      Serial.print("[D] an");
      Serial.print(id);
      Serial.print("=");
      Serial.print(m, 3);
      Serial.println(" m");

      float x, y, z;
      if (solve3DWithMirror(x, y, z)) {
        Serial.print("[POS] x=");
        Serial.print(x, 3);
        Serial.print(" m, y=");
        Serial.print(y, 3);
        Serial.print(" m, z=");
        Serial.print(z, 3);
        Serial.print(" m (");
        Serial.print(x / INCH_TO_M, 2);
        Serial.print(" in, ");
        Serial.print(y / INCH_TO_M, 2);
        Serial.print(" in, ");
        Serial.print(z / INCH_TO_M, 2);
        Serial.println(" in)");
      }
    } else {
      Serial.print("[RAW] ");
      Serial.println(line);
    }
  }
}

