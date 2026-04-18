#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// WiFi credentials
const char* ssid = "ssid";
const char* password = "password";

// Server config
const char* serverUrl = "http://<server-ip>:8000/sensors";
const char* clientSecret = "mysecret";
const char* deviceId = "esp8266d1-ds18b20-01";

// DS18B20 on GPIO14 (D5 on Wemos D1 Mini)
const int oneWireBus = 14;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

WiFiClient wifiClient;


void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== ESP8266 DS18B20 HTTP POST ===");

  // Connect WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  Serial.println("Waiting for network stack to settle...");
  delay(2000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Subnet: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());

  sensors.begin();

  // Verify sensor is found
  int deviceCount = sensors.getDeviceCount();
  Serial.print("DS18B20 devices found: ");
  Serial.println(deviceCount);
  if (deviceCount == 0) {
    Serial.println("WARNING: No DS18B20 detected. Check wiring on GPIO14 (D5).");
  }
}

void postTemperature(float tempC) {
  // Direct TCP test to confirm reachability
  Serial.print("[DEBUG] Free heap: ");
  Serial.println(ESP.getFreeHeap());

  Serial.print("[DEBUG] TCP test to 172.16.0.15:8000... ");
  if (wifiClient.connect("172.16.0.15", 8000)) {
    Serial.println("OK");
    wifiClient.stop();  // close the test connection
    delay(100);
  } else {
    Serial.println("FAILED — server unreachable at TCP level");
    return;
  }

  HTTPClient http;
  http.setTimeout(10000);  // 10s timeout, default 5s can be tight

  // begin() with WiFiClient reference — this is the ESP8266-correct form
  bool begun = http.begin(wifiClient, serverUrl);
  if (!begun) {
    Serial.println("[HTTP] http.begin() failed — check URL format");
    return;
  }

  // Headers
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-client-secret", clientSecret);

  // Build JSON payload
  unsigned long timestampMs = millis();  // substitute with NTP epoch ms if you add time sync
  String payload = "{";
  payload += "\"device_id\":\"" + String(deviceId) + "\",";
  payload += "\"timestamp_ms\":" + String(timestampMs) + ",";
  payload += "\"temperature_c\":" + String(tempC, 2);
  payload += "}";

  Serial.print("[HTTP] POST payload: ");
  Serial.println(payload);

  Serial.println("Waiting for network stack to settle...");
  delay(2000);
  int httpCode = http.POST(payload);

  // httpCode < 0 means a transport-level error (DNS, connection refused, timeout)
  if (httpCode > 0) {
    Serial.printf("[HTTP] Response code: %d\n", httpCode);
    String response = http.getString();
    Serial.print("[HTTP] Response body: ");
    Serial.println(response);
  } else {
    Serial.printf("[HTTP] POST failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  Serial.println("[HTTP] Connection closed");
}

void loop() {
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);

  // -127 means read failure
  if (temperatureC == DEVICE_DISCONNECTED_C) {
    Serial.println("[SENSOR] Read failed (-127). Check wiring.");
  } else {
    Serial.printf("[SENSOR] Temperature: %.2f °C\n", temperatureC);
    postTemperature(temperatureC);
  }

  delay(5000);
}