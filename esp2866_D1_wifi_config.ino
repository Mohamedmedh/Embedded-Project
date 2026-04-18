

#include <ESP8266WiFi.h>

const char* AP_SSID = "MazeRobot";
const char* AP_PASS = "12345678";
const int   PORT    = 8888;

WiFiServer server(PORT);
WiFiClient client;

void setup() {
  Serial.begin(9600);   // UART to Arduino Mega (Serial1 on Mega)
  delay(100);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  server.begin();
  server.setNoDelay(true);

  // Blink LED to show ready
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_BUILTIN, i % 2 == 0 ? LOW : HIGH);
    delay(200);
  }
}

void loop() {
  // Accept new client connection
  if (server.hasClient()) {
    if (client && client.connected()) {
      client.stop();
    }
    client = server.available();
    digitalWrite(LED_BUILTIN, LOW);   // LED on = client connected
  }

  if (!client || !client.connected()) {
    digitalWrite(LED_BUILTIN, HIGH);  // LED off = no client
    return;
  }

  // WiFi → Serial  (Python → Arduino Mega)
  while (client.available()) {
    Serial.write(client.read());
  }

  // Serial → WiFi  (Arduino Mega → Python)
  while (Serial.available()) {
    client.write(Serial.read());
  }
}