#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "ESP32-Joystick-AP2";
const char* password = "123456789";

#define JOY_X_PIN 34
#define JOY_Y_PIN 35

AsyncWebServer server(80);

int xVal = 0, yVal = 0;

void setup() {
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  // Endpoint to fetch joystick values
  server.on("/joystick", HTTP_GET, [](AsyncWebServerRequest *request){
    String data = String(xVal) + "," + String(yVal);
    request->send(200, "text/plain", data);
  });

  // Simple webpage with auto-update
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String page = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <meta charset="utf-8">
        <title>Joystick Debug</title>
        <script>
          async function updateValues() {
            const res = await fetch('/joystick');
            const txt = await res.text();
            document.getElementById('vals').innerText = txt;
          }
          setInterval(updateValues, 100); // update every 100 ms
        </script>
      </head>
      <body>
        <h2>Joystick Values</h2>
        <div id="vals">Loading...</div>
      </body>
      </html>
    )rawliteral";
    request->send(200, "text/html", page);
  });

  server.begin();
}

void loop() {
  int rawX = analogRead(JOY_X_PIN);
  int rawY = analogRead(JOY_Y_PIN);

  xVal = map(rawX, 0, 4095, -1000, 1000);
  yVal = map(rawY, 0, 4095, -1000, 1000);

  delay(20); // ~50 Hz updates (fast enough for robot + display)
}
