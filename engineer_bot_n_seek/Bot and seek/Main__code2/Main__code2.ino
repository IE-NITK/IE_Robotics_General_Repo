#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
const char* ssid     = "ESP32-Joystick-AP2";
const char* password = "123456789";

// Server URL for joystick values (x,y)
const char* serverURL = "http://192.168.4.1/joystick";

// Motor driver pins (L298 style or similar)
#define ENA 12   // PWM pin for motor A
#define IN1 27
#define IN2 14
#define ENB 33   // PWM pin for motor B
#define IN3 26
#define IN4 25

// LEDC configuration
const uint32_t PWM_FREQ = 2000;    // 2 kHz PWM (you may choose another)
const uint8_t  PWM_RES  = 8;       // 8-bit resolution (0–255)

// Optionally choose explicit LEDC channels
const int LEDC_CHANNEL_A = 0;
const int LEDC_CHANNEL_B = 1;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nConnected to Joystick AP!");
  Serial.print("My IP: ");
  Serial.println(WiFi.localIP());

  // Direction pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Setup LEDC for ENA and ENB
  // Option A: Use ledcAttach (auto channel)
  // ledcAttach(ENA, PWM_FREQ, PWM_RES);
  // ledcAttach(ENB, PWM_FREQ, PWM_RES);

  // Option B: Use ledcAttachChannel explicitly
  ledcAttachChannel(ENA, PWM_FREQ, PWM_RES, LEDC_CHANNEL_A);
  ledcAttachChannel(ENB, PWM_FREQ, PWM_RES, LEDC_CHANNEL_B);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      int sep = payload.indexOf(',');
      int x = payload.substring(0, sep).toInt();
      int y = payload.substring(sep + 1).toInt();

      Serial.printf("Joystick: x=%d, y=%d\n", x, y);
      driveMotors(x, y);
    } else {
      Serial.printf("HTTP Error: %d\n", httpCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected!");
  }

  delay(50);
}

int mapValue(int x, int in_min, int in_max, int out_min, int out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void driveMotors(int vx, int vy) {
  vx = constrain(vx, -1000, 1000); // forward/backward
  vy = constrain(vy, -1000, 1000); // left/right

  // Mix joystick values for differential drive
  int rightSpeed  = vy - vx;
  int leftSpeed = vx + vy;

  // Constrain to valid range
  leftSpeed  = constrain(leftSpeed, -1000, 1000);
  rightSpeed = constrain(rightSpeed, -1000, 1000);

  // Map to 8-bit PWM (0–255)
  int leftPWM  = mapValue(abs(leftSpeed), 0, 1000, 0, 200);
  int rightPWM = mapValue(abs(rightSpeed), 0, 1000, 0, 200);

  // Set motor A (left)
  if (leftSpeed > 50) {           // forward
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWrite(ENA, leftPWM);
  } else if (leftSpeed < -50) {   // backward
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    ledcWrite(ENA, leftPWM);
  } else {                        // stop
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    ledcWrite(ENA, 0);
  }

  // Set motor B (right)
  if (rightSpeed > 50) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    ledcWrite(ENB, rightPWM);
  } else if (rightSpeed < -50) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    ledcWrite(ENB, rightPWM);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    ledcWrite(ENB, 0);
  }
}