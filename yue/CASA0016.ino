#include <Wire.h>
#include <Adafruit_SCD30.h>
#include <Adafruit_SSD1306.h>

// Initialize SCD30 and OLED
Adafruit_SCD30 scd30;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Buzzer pin
#define BUZZER_PIN 6

// Alarm counters
int count1000 = 0;      // Consecutive readings above 1000 ppm
int count2000 = 0;      // Consecutive readings above 2000 ppm
const int thresholdCount = 3; // Threshold for consecutive high readings
int alarmCount = 0;    // Buzzer alarm counter
const int maxAlarms = 5; // Maximum allowed alarms before a cooldown
unsigned long lastAlarmTime = 0; // Last alarm time
const unsigned long alarmResetTime = 300000; // 5-minute cooldown in milliseconds

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  // Initialize SCD30
  if (!scd30.begin()) {
    Serial.println("Failed to find SCD30 chip");
    while (1) { delay(10); }
  }
  Serial.println("SCD30 Found!");

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();

  // Initialize buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH); // Ensure buzzer starts silent

  // Display measurement interval
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Interval: ");
  display.print(scd30.getMeasurementInterval());
  display.println(" sec");
  display.display();
  delay(2000);
}

void loop() {
  if (scd30.dataReady()) {
    // Read sensor data
    if (!scd30.read()) {
      Serial.println("Error reading sensor data");
      return;
    }

    // Print data to Serial Monitor
    Serial.print("Temperature: ");
    Serial.print(scd30.temperature);
    Serial.println(" degrees C");

    Serial.print("Relative Humidity: ");
    Serial.print(scd30.relative_humidity);
    Serial.println(" %");

    Serial.print("CO2: ");
    Serial.print(scd30.CO2, 3);
    Serial.println(" ppm");
    Serial.println("");

    // Update OLED display
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Temp: ");
    display.print(scd30.temperature);
    display.println(" C");

    display.setCursor(0, 16);
    display.print("Humidity: ");
    display.print(scd30.relative_humidity);
    display.println(" %");

    display.setCursor(0, 32);
    display.print("CO2: ");
    display.print(scd30.CO2, 3);
    display.println(" ppm");

    // Buzzer control logic
    if (millis() - lastAlarmTime > alarmResetTime) {
      alarmCount = 0; // Reset alarm count after cooldown
    }

    if (alarmCount < maxAlarms) {
      if (scd30.CO2 > 2000) {
        count2000++;
        count1000 = 0;
        if (count2000 >= thresholdCount) {
          digitalWrite(BUZZER_PIN, LOW); // Long beep
          delay(1000);
          digitalWrite(BUZZER_PIN, HIGH);
          alarmCount++;
          lastAlarmTime = millis();
        }
      } else if (scd30.CO2 > 1000) {
        count1000++;
        count2000 = 0;
        if (count1000 >= thresholdCount) {
          digitalWrite(BUZZER_PIN, LOW); // Short beep
          delay(200);
          digitalWrite(BUZZER_PIN, HIGH);
          delay(800);
          alarmCount++;
          lastAlarmTime = millis();
        }
      } else {
        count1000 = 0;
        count2000 = 0;
      }
    }

    // Display warning icon and text if alarms triggered
    if (alarmCount > 0) {
      display.fillTriangle(0, 56, 10, 56, 5, 48, SSD1306_WHITE);
      const char *warningText = "Warning!!";
      int adjustedPosition = (SCREEN_WIDTH - (9 * 6)) / 2 - ((SCREEN_WIDTH - (9 * 6)) / 4);
      display.setCursor(adjustedPosition, 48);
      display.println(warningText);
    }

    display.display();
  }

  delay(2000); // Update every 2 seconds
}
