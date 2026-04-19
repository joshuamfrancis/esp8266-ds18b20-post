# ESP8266 DS18B20 HTTP POST

This repository contains an ESP8266 sketch that reads temperature from a DS18B20 sensor and sends it to an HTTP server every 5 minutes.

## Hardware Wiring

Use a Wemos D1 Mini / ESP8266 board and wire the DS18B20 as follows:

- DS18B20 VCC -> 3.3V
- DS18B20 GND -> GND
- DS18B20 DATA -> GPIO14 (D5 on Wemos D1 Mini)
- 4.7K resistor between DS18B20 DATA and 3.3V

> The sketch uses `TEMP_PIN = 14`, which is D5 on the Wemos D1 Mini.

## Required Libraries

Install these Arduino libraries:

- OneWire
- DallasTemperature
- ESP8266WiFi
- ESP8266HTTPClient

## Configuration

Open `esp8266_PostHttpClient/esp8266_PostHttpClient.ino` and update the configuration section:

- `WIFI_SSID` - your WiFi network name
- `WIFI_PASSWORD` - your WiFi password
- `SERVER_DOMAIN` - hostname of your server (for example `labpc.home`)
- `SERVER_PORT` - server port (for example `8000`)
- `SERVER_PATH` - endpoint path (for example `/sensors`)
- `CLIENT_SECRET` - shared secret header value
- `DEVICE_ID` - a unique device identifier

The sketch sends JSON with `device_id`, `timestamp_ms`, and `temperature_c`.

## Secret Setup

The sketch sends the shared secret in the HTTP header `x-client-secret`.

Generate a secret token with:

```bash
openssl rand -hex 10
```

Use the generated value for `CLIENT_SECRET` in the sketch and configure your server to expect the same value.

Example:

```c++
const char* CLIENT_SECRET = "your_generated_secret_here";
```

## Notes

- The sketch performs a DNS lookup for `SERVER_DOMAIN` and posts to `http://<resolved-ip>:<SERVER_PORT><SERVER_PATH>`.
- If the DS18B20 sensor read fails, the serial monitor prints a warning.
- The temperature is posted every 5 minutes by default (`LOOP_DELAY_MS = 300000`).

## Build

Use the Arduino IDE or PlatformIO with the ESP8266 board package installed. Upload the sketch after updating the configuration values.
