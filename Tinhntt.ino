#define BLYNK_TEMPLATE_ID "TMPL6jj63m89B"
#define BLYNK_TEMPLATE_NAME "Auto"
#define BLYNK_AUTH_TOKEN "yBz7Ie0XV39QXMtyC834wtqZH4jiDDr5"

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// WiFi Credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Wifi Username";  // Enter your Wifi Username
char pass[] = "Wifi Password";  // Enter your Wifi password
// Pin Configuration
#define LIGHT_SENSOR_PIN A0
#define BUTTON_PIN D0
#define LEDPIN D4

BlynkTimer timer;

// System Status Variables
bool autoMode = false;
bool lightStatus = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Light Control Variables
int lightThreshold = 300;  // Default threshold
int hysteresis = 50;       // Prevent flickering

void checkButton();
void updateLight(bool newStatus, String source);
void checkLightCondition();

BLYNK_WRITE(V0) { // Auto Mode Toggle
  autoMode = param.asInt();
  Serial.print("Auto Mode ");
  Serial.println(autoMode ? "ENABLED" : "DISABLED");
  
  // Disable manual control in auto mode
  Blynk.setProperty(V1, "isDisabled", autoMode ? "true" : "false");
  
  // Update light immediately when mode changes
  checkLightCondition();
}

BLYNK_WRITE(V1) { // Manual Light Control
  if (!autoMode) { // Only allow manual control when not in auto mode
    updateLight(param.asInt(), "Blynk Manual");
  }
}

void updateLight(bool newStatus, String source) {
  if (lightStatus != newStatus) {
    lightStatus = newStatus;
    digitalWrite(LEDPIN, lightStatus ? HIGH : LOW);
    Serial.print("Light ");
    Serial.print(lightStatus ? "ON" : "OFF");
    Serial.print(" by ");
    Serial.println(source);
    
    // Always sync with Blynk app
    Blynk.virtualWrite(V1, lightStatus);
  }
}

void checkLightCondition() {
  if (autoMode) {
    int lightValue = analogRead(LIGHT_SENSOR_PIN);
    Serial.print("Light Sensor: ");
    Serial.print(lightValue);
    Serial.print(" | Threshold: ");
    Serial.println(lightThreshold);

    // Dark condition (LOW value = dark)
    if (lightValue < (lightThreshold - hysteresis)) {
      updateLight(true, "Auto Dark");
    } 
    // Bright condition (HIGH value = bright)
    else if (lightValue > (lightThreshold + hysteresis)) {
      updateLight(false, "Auto Bright");
    }
  }
}

void checkButton() {
  int reading = digitalRead(BUTTON_PIN);
  
  // Debounce logic
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW) { // Button pressed
      autoMode = !autoMode;
      Blynk.virtualWrite(V0, autoMode);
      Blynk.setProperty(V1, "isDisabled", autoMode ? "true" : "false");
      
      Serial.print("Button pressed. Auto Mode ");
      Serial.println(autoMode ? "ON" : "OFF");
      
      checkLightCondition();
    }
  }
  
  lastButtonState = reading;
}

void setup() {
  Serial.begin(115200);
  pinMode(LEDPIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
digitalWrite(LEDPIN, LOW);

  // Initialize WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Initialize Blynk
  Blynk.begin(auth, ssid, pass);
  Serial.println("Blynk Connected!");

  // Sync initial states
  Blynk.virtualWrite(V0, autoMode);
  Blynk.virtualWrite(V1, lightStatus);
  Blynk.setProperty(V1, "label", autoMode ? "Light (Auto)" : "Light (Manual)");

  // Setup timer
  timer.setInterval(1000L, []() {
    checkButton();
    checkLightCondition();
  });
}

void loop() {
  Blynk.run();
  timer.run();
}