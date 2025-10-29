/* 
  Earthquake Detection System (ESP32 + MPU6050 + OLED + Twilio SMS)
  - Calibrates MPU6050 on boot
  - Monitors acceleration magnitude and classifies events:
      Minor  (5 - 10 m/s^2)  -> Green LED + OLED message
      Moderate (10 - 15 m/s^2) -> Yellow LED + pulsed buzzer + SMS
      Extreme (> 15 m/s^2) -> Red LED + continuous buzzer + SMS
  - 30 second cooldown for outgoing SMS to avoid spamming
  - Use secure storage for credentials before publishing
  Libraries required:
    - Adafruit MPU6050
    - Adafruit SSD1306
    - Adafruit GFX
    - WiFi
    - HTTPClient
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

const char* TWILIO_SID   = "ACXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
const char* TWILIO_TOKEN = "your_auth_token_here";
const char* TWILIO_FROM  = "+1XXXXXXXXXX"; 
const char* TWILIO_TO    = "+91XXXXXXXXXX"; 

const unsigned long SMS_COOLDOWN_MS = 30UL * 1000UL; 

const float THRESH_MINOR    = 5.0;
const float THRESH_MODERATE = 10.0;
const float THRESH_EXTREME  = 15.0;

#define GREEN_LED_PIN   18
#define YELLOW_LED_PIN  19
#define RED_LED_PIN     23
#define BUZZER_PIN      5

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_MPU6050 mpu;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

float offsetX = 0.0, offsetY = 0.0, offsetZ = 0.0;
unsigned long lastSmsMillis = 0;

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  noTone(BUZZER_PIN);

  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ERROR: SSD1306 not found");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Earthquake System");
    display.display();
  }

  if (!mpu.begin()) {
    Serial.println("ERROR: MPU6050 not found");
    while (1) { delay(500); } 
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  
  connectWiFi();

  
  calibrateMPU();

  
  showOLEDStatus("System Ready", "Calibrated");
  delay(1000);
}

void loop() {
  
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  
  float ax = a.acceleration.x - offsetX;
  float ay = a.acceleration.y - offsetY;
  float az = a.acceleration.z - offsetZ;

  
  float magnitude = sqrt(ax*ax + ay*ay + az*az);

  
  Serial.print("Mag: ");
  Serial.print(magnitude, 3);
  Serial.print(" | ax=");
  Serial.print(ax, 3);
  Serial.print(" ay=");
  Serial.print(ay, 3);
  Serial.print(" az=");
  Serial.println(az, 3);

  
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  noTone(BUZZER_PIN);

  
  if (magnitude > THRESH_EXTREME) {
    // Extreme
    digitalWrite(RED_LED_PIN, HIGH);
    tone(BUZZER_PIN, 2000); 
    showOLEDStatus("EXTREME QUAKE", String(magnitude,2));
    trySendSMS("EXTREME Earthquake detected! Magnitude: " + String(magnitude, 2));
  } else if (magnitude > THRESH_MODERATE) {
    // Moderate
    digitalWrite(YELLOW_LED_PIN, HIGH);
    //  buzzer
    tone(BUZZER_PIN, 1200);
    delay(300);
    noTone(BUZZER_PIN);
    delay(200);
    tone(BUZZER_PIN, 1200);
    delay(300);
    noTone(BUZZER_PIN);
    showOLEDStatus("MODERATE QUAKE", String(magnitude,2));
    trySendSMS("MODERATE Earthquake detected! Magnitude: " + String(magnitude, 2));
  } else if (magnitude > THRESH_MINOR) {
    // Minor
    digitalWrite(GREEN_LED_PIN, HIGH);
    showOLEDStatus("MINOR SHAKE", String(magnitude,2));
  } else {
    // Stable
    showOLEDStatus("STABLE", String(magnitude,2));
  }

  
  delay(50);
}



void connectWiFi() {
  Serial.print("Connecting to WiFi ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
    if (millis() - start > 15000) { 
      Serial.println("\nWiFi connect timeout. Continuing without WiFi.");
      return;
    }
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
}

void calibrateMPU() {
  Serial.println("Calibrating MPU6050...");
  const int samples = 200;
  float sx = 0, sy = 0, sz = 0;
  for (int i = 0; i < samples; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    sx += a.acceleration.x;
    sy += a.acceleration.y;
    sz += a.acceleration.z;
    delay(10);
  }
  offsetX = sx / samples;
  offsetY = sy / samples;
  offsetZ = sz / samples;
  Serial.print("Offsets -> X: "); Serial.print(offsetX,4);
  Serial.print(" Y: "); Serial.print(offsetY,4);
  Serial.print(" Z: "); Serial.println(offsetZ,4);

  
}

void showOLEDStatus(const String &line1, const String &line2) {
  if (!display.display()) {
   
  }
  if (display.width() == 0) return; 
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Earthquake Monitor");
  display.println("------------------");
  display.setTextSize(2);
  display.println();
  display.println(line1);
  display.setTextSize(1);
  display.println();
  display.print("Mag: ");
  display.println(line2);
  display.display();
}

String urlEncode(const String &str) {
  String encoded = "";
  char c;
  char buf[4];
  for (size_t i = 0; i < str.length(); i++) {
    c = str[i];
    if ( (c >= '0' && c <= '9') ||
         (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      sprintf(buf, "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}

void trySendSMS(const String &message) {
  
  unsigned long now = millis();
  if (now - lastSmsMillis < SMS_COOLDOWN_MS) {
    Serial.println("SMS cooldown - skipping SMS");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected - cannot send SMS");
    return;
  }
  
  bool ok = sendTwilioSMS(TWILIO_TO, TWILIO_FROM, message);
  if (ok) {
    lastSmsMillis = now;
    Serial.println("SMS sent. Cooldown started.");
  } else {
    Serial.println("SMS failed.");
  }
}

bool sendTwilioSMS(const char* to, const char* from, const String &body) {
  HTTPClient http;
  String url = "https://api.twilio.com/2010-04-01/Accounts/";
  url += TWILIO_SID;
  url += "/Messages.json";

 
  String postData = "To=" + urlEncode(String(to));
  postData += "&From=" + urlEncode(String(from));
  postData += "&Body=" + urlEncode(body);

  http.begin(url);
  
  http.setAuthorization(TWILIO_SID, TWILIO_TOKEN);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(postData);
  Serial.print("Twilio HTTP code: ");
  Serial.println(httpCode);
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Twilio response:");
    Serial.println(payload);
  } else {
    Serial.print("HTTP POST failed, error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
  return (httpCode == 201 || httpCode == 200);
}
