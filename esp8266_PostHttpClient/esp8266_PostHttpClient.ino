#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

// ── Configuration ────────────────────────────────────────────────────────────
const char* WIFI_SSID       = "xxxxxxx";
const char* WIFI_PASSWORD   = "xxxxxxx";

const char* SERVER_DOMAIN   = "labpc.home";
const char* SERVER_PATH     = "/sensors";
const int   SERVER_PORT     = 8000;

const char* CLIENT_SECRET   = "xxxxxxx"; // Must match server's expected value
const char* DEVICE_ID       = "esp8266d1-ds18b20-01";

const int   TEMP_PIN        = 14;   // GPIO14 = D5 on Wemos D1 Mini
const int   LOOP_DELAY_MS   = 300000; //every 5 minutes
// ─────────────────────────────────────────────────────────────────────────────

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
WiFiClient wifiClient;


// ── DNS Diag──────────────────────────────────────────────────────────────────
void rawDnsTest() {
  WiFiUDP udp;
  udp.begin(12345);

  // Minimal DNS query for your hostname
  // Change "yourserver" to match your actual hostname
  const char* hostname = "labpc.home";
  uint8_t query[29] = {
    0x00, 0x01,  // Transaction ID
    0x01, 0x00,  // Flags: standard query
    0x00, 0x01,  // Questions: 1
    0x00, 0x00,  // Answer RRs: 0
    0x00, 0x00,  // Authority RRs: 0
    0x00, 0x00,  // Additional RRs: 0
  };

  // This is a simplified test — just check if Pi-hole responds at all
  udp.beginPacket(WiFi.dnsIP(), 53);
  udp.write(query, sizeof(query));
  int sent = udp.endPacket();
  Serial.printf("[RAW DNS] Packet sent: %d\n", sent);

  delay(2000);
  int size = udp.parsePacket();
  Serial.printf("[RAW DNS] Response size: %d bytes\n", size);
  if (size > 0) {
    uint8_t buf[64];
    udp.read(buf, size);
    Serial.printf("[RAW DNS] Response size: %d bytes\n", size);
    // Print response flags (byte 3 contains rcode)
    Serial.printf("[RAW DNS] Flags: 0x%02X%02X\n", buf[2], buf[3]);
    Serial.printf("[RAW DNS] RCODE: %d\n", buf[3] & 0x0F);
    // RCODE 0 = success, 1 = format error, 2 = server fail, 3 = NXDOMAIN
  } else {
    Serial.println("[RAW DNS] No response — Pi-hole not reachable on UDP port 53");
  }
  udp.stop();
}


// ── WiFi ─────────────────────────────────────────────────────────────────────
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.printf("Connected! IP: %s  DNS: %s\n",
    WiFi.localIP().toString().c_str(),
    WiFi.dnsIP().toString().c_str());
}

// ── DNS ──────────────────────────────────────────────────────────────────────
bool resolveHost(IPAddress& out) {
  // Give lwIP time to actually process the response
  for (int attempt = 1; attempt <= 3; attempt++) {
    int result = WiFi.hostByName(SERVER_DOMAIN, out, 10000);  // 10s timeout
    if (result == 1) {
      Serial.printf("[DNS] %s → %s\n", SERVER_DOMAIN, out.toString().c_str());
      return true;
    }
    Serial.printf("[DNS] Attempt %d result code: %d\n", attempt, result);
    delay(2000);
    yield();  // let lwIP process pending packets
  }
  return false;
}
// ── HTTP POST ────────────────────────────────────────────────────────────────
void postTemperature(float tempC) {
  IPAddress ip;
  if (!resolveHost(ip)) return;

  String url = "http://" + ip.toString() + ":" + SERVER_PORT + SERVER_PATH;

  String payload = "{";
  payload += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  payload += "\"timestamp_ms\":" + String(millis()) + ",";
  payload += "\"temperature_c\":" + String(tempC, 2);
  payload += "}";

  HTTPClient http;
  http.setTimeout(10000);
  http.begin(wifiClient, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-client-secret", CLIENT_SECRET);

  Serial.printf("[HTTP] POST %s  payload: %s\n", url.c_str(), payload.c_str());

  int code = http.POST(payload);
  if (code > 0) {
    Serial.printf("[HTTP] %d  %s\n", code, http.getString().c_str());
  } else {
    Serial.printf("[HTTP] Error: %s\n", http.errorToString(code).c_str());
  }

  http.end();
}

// ── Setup / Loop ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== ESP8266 DS18B20 HTTP POST ===");

  connectWiFi();

  rawDnsTest();
  sensors.begin();
  Serial.printf("DS18B20 devices found: %d\n", sensors.getDeviceCount());
}

void loop() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("[SENSOR] Read failed. Check wiring on GPIO14 (D5).");
  } else {
    Serial.printf("[SENSOR] %.2f °C\n", tempC);
    postTemperature(tempC);
  }

  delay(LOOP_DELAY_MS);
}