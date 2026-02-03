#include "app.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "commands.h"
#include "config.h"
#include "mavlink_iface.h"
#include "telemetry.h"
#include "web_server.h"

static uint32_t lastHeartbeat = 0;
static uint32_t lastSetpoint = 0;
static WiFiUDP rtkUdp;
static bool rtkUdpReady = false;

static bool motherTargetActive = false;
static double motherTargetLat = 0;
static double motherTargetLon = 0;
static float motherTargetRelAlt = 0;

static uint32_t childCmdSeq = 0;
static String childCmdMode = "goto";
static double childTargetLat = 0;
static double childTargetLon = 0;
static float childTargetRelAlt = 3.0f;
static float childTargetYawDeg = 0.0f;
static float childOffsetNorth = 0.0f;
static float childOffsetEast = 0.0f;
static float childAltOffset = 0.0f;

void cmdMotherHover(float relAlt) {
  if (!mother.valid) return;
  motherTargetLat = mother.lat;
  motherTargetLon = mother.lon;
  motherTargetRelAlt = relAlt;
  motherTargetActive = true;
}

void cmdMotherArm(bool arm) {
  mavlinkSendArmDisarm(arm);
}

void cmdMotherRtl() {
  if (!mother.valid) return;
  motherTargetLat = mother.lat;
  motherTargetLon = mother.lon;
  motherTargetRelAlt = 0.0f;
  motherTargetActive = true;
}

static void sendChildGoto(double lat, double lon, float relAlt) {
  childTargetLat = lat;
  childTargetLon = lon;
  childTargetRelAlt = relAlt;
  childCmdMode = "goto";
  childCmdSeq++;
}

static void sendChildDock(double lat, double lon, float relAlt) {
  childTargetLat = lat;
  childTargetLon = lon;
  childTargetRelAlt = relAlt;
  childTargetYawDeg = mother.heading;
  childCmdMode = "dock";
  childCmdSeq++;
}

void cmdChildDock() {
  if (!mother.valid) return;
  const double latRad = mother.lat * DEG_TO_RAD;
  const double dLat = childOffsetNorth / 111111.0;
  const double dLon = childOffsetEast / (111111.0 * cos(latRad));
  const double lat = mother.lat + dLat;
  const double lon = mother.lon + dLon;
  const float relAlt = 3.0f + childAltOffset;
  sendChildDock(lat, lon, relAlt);
}

void cmdChildRtl() {
  childCmdMode = "rtl";
  childCmdSeq++;
}

void cmdChildAlt(float delta) {
  childAltOffset += delta;
  cmdChildDock();
}

void cmdChildMove(float dn, float de) {
  childOffsetNorth += dn;
  childOffsetEast += de;
  cmdChildDock();
}

String buildChildCmdResponse() {
  String resp = "seq=" + String(childCmdSeq) +
                ";cmd=" + childCmdMode +
                ";lat=" + String(childTargetLat, 7) +
                ";lon=" + String(childTargetLon, 7) +
                ";alt=" + String(childTargetRelAlt, 2) +
                ";yaw=" + String(childTargetYawDeg, 1) + ";";
  return resp;
}

void appSetup() {
  Serial.begin(115200);
  delay(200);

  mavlinkSetup();

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GW, AP_SUBNET);
  WiFi.softAP(AP_SSID, AP_PASS);

  rtkUdp.begin(RTCM_UDP_PORT);
  rtkUdpReady = true;

  webSetup();
  mavlinkRequestIntervals();
}

void appLoop() {
  webLoop();

  if (millis() - lastHeartbeat > 1000) {
    lastHeartbeat = millis();
    mavlinkSendHeartbeat();
  }

  mavlinkLoop();

  if (rtkUdpReady) {
    int packetSize = rtkUdp.parsePacket();
    while (packetSize > 0) {
      uint8_t buf[512];
      int len = rtkUdp.read(buf, sizeof(buf));
      if (len > 0) {
        mavlinkWriteRaw(buf, (size_t)len);
      }
      packetSize = rtkUdp.parsePacket();
    }
  }

  if (motherTargetActive && mother.valid && (millis() - lastSetpoint > 200)) {
    lastSetpoint = millis();
    mavlinkSendSetpointGlobalRelAlt(motherTargetLat, motherTargetLon, motherTargetRelAlt);
  }
}

