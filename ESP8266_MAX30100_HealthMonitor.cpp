// IoT-based Health Monitoring System using NodeMCU ESP8266 and MAX30100 sensor
// This project is designed to monitor heart rate and SpO2 levels, sending the data to the Blynk app for real-time monitoring.

// Define Blynk project settings
#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "IoT Project"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

// Include necessary libraries
#include <Wire.h>                   // Allows communication with I2C devices
#include "MAX30100_PulseOximeter.h" // Library for the MAX30100 sensor
#define BLYNK_PRINT Serial          // Directs Blynk output to the serial monitor
#include <BlynkSimpleEsp8266.h>     // Blynk library for ESP8266
#include <ESP8266WiFi.h>            // Enables ESP8266 to connect to WiFi network
#include "Adafruit_GFX.h"           // Base class for graphics
#include "OakOLED.h"                // OLED display library

#define REPORTING_PERIOD_MS 1000 // Data reporting period in milliseconds
OakOLED oled;                    // Create OLED display object

// WiFi credentials (replace with your own)
char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASS";
char auth[] = BLYNK_AUTH_TOKEN; // Blynk authorization token

const int buzzerPin = D3; // Pin connected to the buzzer

// Initialize PulseOximeter instance
PulseOximeter pox;

// Variables to store heart rate and SpO2 readings
float BPM, SpO2;
uint32_t tsLastReport = 0; // Last time data was sent to Blynk

// Bitmap for heart image representation on OLED
const unsigned char bitmap[] PROGMEM =
    {
        // Bitmap array truncated for brevity
        0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x18, 0x00, 0x0f, 0xe0, 0x7f, 0x00, 0x3f, 0xf9, 0xff, 0xc0,
        0x7f, 0xf9, 0xff, 0xc0, 0x7f, 0xff, 0xff, 0xe0, 0x7f, 0xff, 0xff, 0xe0, 0xff, 0xff, 0xff, 0xf0,
        0xff, 0xf7, 0xff, 0xf0, 0xff, 0xe7, 0xff, 0xf0, 0xff, 0xe7, 0xff, 0xf0, 0x7f, 0xdb, 0xff, 0xe0,
        0x7f, 0x9b, 0xff, 0xe0, 0x00, 0x3b, 0xc0, 0x00, 0x3f, 0xf9, 0x9f, 0xc0, 0x3f, 0xfd, 0xbf, 0xc0,
        0x1f, 0xfd, 0xbf, 0x80, 0x0f, 0xfd, 0x7f, 0x00, 0x07, 0xfe, 0x7e, 0x00, 0x03, 0xfe, 0xfc, 0x00,
        0x01, 0xff, 0xf8, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x3f, 0xc0, 0x00,
        0x00, 0x0f, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Callback function to be called when a heartbeat is detected
void onBeatDetected()
{
    Serial.println("Beat Detected!");
    oled.drawBitmap(60, 20, bitmap, 28, 28, 1);
    oled.display();
}

void setup()
{
    Serial.begin(115200); // Start serial communication at 115200 baud
    oled.begin();         // Initialize OLED display
    oled.clearDisplay();  // Clear OLED display
    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 0);

    // Display initializing message on OLED
    oled.println("Initializing pulse oximeter..");
    oled.display();

    // Initialize pins
    pinMode(16, OUTPUT);
    pinMode(buzzerPin, OUTPUT);

    // Connect to WiFi and Blynk
    Blynk.begin(auth, ssid, pass);

    Serial.print("Initializing Pulse Oximeter..");

    // Attempt to initialize MAX30100
    if (!pox.begin())
    {
        Serial.println("FAILED");
        oled.clearDisplay();
        oled.setTextSize(1);
        oled.setTextColor(1);
        oled.setCursor(0, 0);
        oled.println("FAILED");
        oled.display();
        for (;;)
            ; // Halt execution
    }
    else
    {
        Serial.println("SUCCESS");
        oled.clearDisplay();
        oled.println("SUCCESS");
        oled.display();
        pox.setOnBeatDetectedCallback(onBeatDetected); // Set heartbeat detected callback
    }
}

void loop()
{
    pox.update(); // Update the PulseOximeter
    Blynk.run();  // Run Blynk

    // Read heart rate and SpO2 every REPORTING_PERIOD_MS
    BPM = pox.getHeartRate();
    SpO2 = pox.getSpO2();
    if (millis() - tsLastReport > REPORTING_PERIOD_MS)
    {
        // Print readings to serial
        Serial.print("Heart rate:");
        Serial.print(BPM);
        Serial.print(" bpm / SpO2:");
        Serial.print(SpO2);
        Serial.println(" %");

        // Activate buzzer if heart rate exceeds threshold
        if (BPM >= 120)
        {
            digitalWrite(buzzerPin, HIGH); // Turn on the buzzer
        }
        else
        {
            digitalWrite(buzzerPin, LOW); // Turn off the buzzer
        }

        // Send data to Blynk
        Blynk.virtualWrite(V1, BPM);
        Blynk.virtualWrite(V2, SpO2);

        // Update OLED display with new readings
        oled.clearDisplay();
        oled.setCursor(0, 0);
        oled.println("Heart BPM");
        oled.println(pox.getHeartRate());
        oled.println("Spo2");
        oled.println(pox.getSpO2());
        oled.display();

        tsLastReport = millis(); // Update last report time
    }
}

// Blynk virtual pin write handler to control buzzer
BLYNK_WRITE(V3)
{
    int buzzerControl = param.asInt();                   // Get button state from Blynk app
    digitalWrite(buzzerPin, buzzerControl ? HIGH : LOW); // Control buzzer based on Blynk app button
}