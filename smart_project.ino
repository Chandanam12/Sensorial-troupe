
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "thingProperties.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>

// Adafruit SSD1306 - Version: Latest
#include <Adafruit_SSD1306.h>
#include <splash.h>

#include <DHT_U.h>

// Wire Library for I2C
#include <Wire.h>


const char* apiKey = "d65e67084d88fea189c5ebffe8ffed0e";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 25
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define SENSOR_IN 34
#define RELAY_OUT 4

const int DryValue = 2700;
const int WetValue = 1800;

float temp;
int moisture_limit;
int soilMoistureValue;
int  soilMoisturePercent;
unsigned long delayStart = 0;
bool delayInProgress = false;
String pump_status_text = "OFF";
int control = 0;
String weatherCondition;




void pumpOn();
void pumpOff();
void updateWeather();
void printOLED(int top_cursor, String top_text, int main_cursor, String main_text, int delay_time);

void setup() {
  Serial.begin(9600);
  delay(1500);
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  Serial.print("WiFi Connection Status: ");
  Serial.println(WiFi.status());

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();

  dht.begin();
  analogReadResolution(12);
  pinMode(RELAY_OUT, OUTPUT);
  digitalWrite(RELAY_OUT, HIGH);
  pump_status = false;
  Serial.print("Received control value: ");
  Serial.println(my_control);
  updateWeather();
  Serial.println("first time update weather completed ");
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
}

void loop() {
  ArduinoCloud.update();
   static unsigned long lastUpdateTime = 0;
  // Check for updates every 5 seconds
    if (millis() - lastUpdateTime >= 300000) {
       updateWeather();
       Serial.println("update weather completed ");
    
    lastUpdateTime = millis();
  }
  

    if (my_control == control) {
      Serial.print("Received option value: ");
      Serial.println(option);
      switch (option) {
        case 0: moisture_limit = 25; break; // wheat
        case 1: moisture_limit = 60; break; // paddy
        case 2: moisture_limit = 50; break; // cotton
        case 3: moisture_limit = 65; break; // maize
        case 4:{ pumpOn();Serial.println("pump on successfull for option value  ");  break;}
        case 5: {pumpOff();Serial.println("pump off sucessfull for option value  "); break;}
        default: Serial.println("Error in finding the crop"); break;
      }

      if (option != 4 && option != 5) {
        if (soilMoisturePercent < moisture_limit && weatherCondition != "Clouds") {
          pumpOn();
          Serial.println("pump on for seleted crop ");
        } else if (soilMoisturePercent < moisture_limit && weatherCondition== "Clouds") {
          if (!delayInProgress) {
            delayStart = millis();
            delayInProgress = true;
          } else if (millis() - delayStart >= 1800000) {
            delayInProgress = false;
            if (soilMoisturePercent < moisture_limit) {
              
              pumpOn();
              Serial.println("pump on sucess after cloudy ");
            }
          }
        } else {
          pumpOff();
        }
      }
      } else {
        if (soilMoisturePercent < my_control && weatherCondition != "Clouds") {
          pumpOn();
          Serial.println("pump on for set moisture value ");
        } else if (soilMoisturePercent < my_control && weatherCondition == "Clouds") {
          if (!delayInProgress) {
            delayStart = millis();
            delayInProgress = true;
          } else if (millis() - delayStart >= 1800000) {
            delayInProgress = false;
            if (soilMoisturePercent < moisture_limit) {
              pumpOn();
              Serial.println("pump on for set moisture value after cloudy ");
            }
          }
        } else {
          pumpOff();
        }
      }

      printOLED(35, "PUMP", 40, pump_status_text, 2000);
      printOLED(35, "TEMP", 10, String(temp) + "C", 2000);
      printOLED(35, "MOIST", 30, String(soilMoisturePercent) + "%", 2000);
      
      
    
    }

  // Your existing code here...

void updateWeather() {
  

    HTTPClient http;
    String city = "Mysore"; // Replace with actual city name or coordinates

    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + String(apiKey);
    http.begin(url);
    
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();

      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      
      
     String weatherCondition = doc["weather"][0]["main"];
      
    temp = dht.readTemperature();
    current_temperature = temp;

    soilMoistureValue = analogRead(SENSOR_IN);
    soilMoisturePercent = map(soilMoistureValue, DryValue, WetValue, 0, 100);
    soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

    Serial.println(soilMoistureValue);
    current_moisture = soilMoisturePercent;
    http.end();
    
}
}

void pumpOn() {
  digitalWrite(RELAY_OUT, LOW);
  pump_status_text = "ON";
  pump_status = true;
  
}

void pumpOff() {
  digitalWrite(RELAY_OUT, HIGH);
  pump_status_text = "OFF";
  pump_status = false;
}

void printOLED(int top_cursor, String top_text, int main_cursor, String main_text, int delay_time) {
  display.setCursor(top_cursor, 0);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println(top_text);

  display.setCursor(main_cursor, 40);
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.print(main_text);
  display.display();

  delay(delay_time);
  display.clearDisplay();
}

void onOptionChange() {
  // Add your code here to handle the change in the option variable
}

void onMyControlChange() {
  // Add your code here to handle the change in the my_control variable
}

void onPumpStatusChange() {
  // Add your code here to handle the change in the pump_status variable
}

 
