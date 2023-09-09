#define Firmware 3.06
#define board_version 1.3
// #define DEBUG

// Iniciate libraries
#include <ESP8266WiFi.h>
#include <Adafruit_INA219.h>
#include <ArduinoJson.h>
#include "SparkFun_Si7021_Breakout_Library.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "LittleFS.h"
#include "logo.h"

// Oled display definitions
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

// Instantiate Sensors
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_INA219 ina219(0x45);
Weather sensor;

// Time related variables
uint32_t mLastTime = 0;
uint32_t sampleLastTime = 0;
uint32_t last_comm = 0;
long lastDebounceTime = 0;
long lastDebounceTime2 = 0;

// GPIO definitions
#define SW1 14
#define SW2 15
#define fan_in 12
#define soc_pw_pin 13
#define i2c_gnd 2

// Control variables
#define debounceDelay 500
#define wait_before_sleep 15
#define samples 31
#define min_fan_speed 400
float min_voltage=6.8; // Voltage when the system should go in protection mode

// SoC power variables
int count_down = wait_before_sleep - 2;
int count_to_wakeup = 0;
bool SoC_PW = true;
long sleep_period = 0;
bool pi_sleeping = false;
long pi_sleep_start = 0;
bool pi_down = false;
bool low_power_detected = false;
long low_power_sleep = 0;
int low_power_counter = 0;
bool power_off_display = false;
long power_cycle_waiting = 0;
int power_cycle_count = 0;
bool power_cycling = false;
bool ckt_hot = false;
bool wakeup = false;

// Array of samples for each sensor
float hum[samples];
float temp[samples];
float current_mA[samples];
float loadvoltage[samples];
float power_mW[samples];
float solar_v[samples];

// Average variable for each sensor
int valid_samples = 1;
float hum_avg;
float temp_avg;
float current_mA_avg;
float loadvoltage_avg;
float power_mW_avg;
float solar_v_avg;
float busvoltage = 0;
float shuntvoltage = 0;

// byte error, address;
// bool booting = false;
// bool fanin = false;

// Define functions for buttons interruption
void ICACHE_RAM_ATTR switch_1();
void ICACHE_RAM_ATTR switch_2();

// Iniciate variable for device name
String DEVICE = "";

// Variables for menu operation
String confirm_change;
long change_time;
bool new_change_state = false;
bool start_change = false;

// Mobile operators options for menu
char *operators[] = {"Vivo","Claro","Tim","Oi","Vodafone","ThingsMobile"};
#define total_operators 6
int op_id = 0;

void setup() {
  // Disable ESP8266 Radio
  WiFi.forceSleepBegin();
  // Set GPIO modes
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(fan_in, OUTPUT);
  pinMode(soc_pw_pin, OUTPUT);
  pinMode(i2c_gnd, OUTPUT);
  // Power up Pi
  pi_power(true);
  // Power up I2C sensors
  i2c_power(true);
  // Set serial
  Serial.begin(115200);
  Serial.println();
  // Initiate ESP filesystem
  if (!LittleFS.begin()) {
    Serial.println("{'error':'An Error has occurred while mounting LittleFS'}");
    return;
  }
  // Call function to update values stored in the filesystem
  update_from_fs();
  // Initiate Oled display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.setRotation(2);
  display.clearDisplay();
  // Call logo, stored in logo.h
  display.drawBitmap(0, 0, bah_TL, 128, 32, WHITE);
  display.display();
  delay(3000);
  // Display device name and firmware version
  if (DEVICE.length() > 8 ) display.setTextSize(2);
  else display.setTextSize(3);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(DEVICE);
  display.setTextSize(1);
  display.setCursor(100, 24);
  display.print(Firmware);
  display.display();

  delay(3000);
  // Iniciate the interruption watch for the buttons
  attachInterrupt(SW1, switch_1, FALLING);
  attachInterrupt(SW2, switch_2, RISING);
  // Initiate the sensors
  sensor.begin();
  // comment line 67 on SparkFun_Si7021_Breakout_Library.cpp to remove HTU21D Found message
  ina219.begin();
  // Print on serial the wake up message
  uc2pi("boot","uc");
  // Start the internal watchdog
  ESP.wdtDisable();
}

void loop() {
  // Feed the watchdog beast
  ESP.wdtFeed();
  // Check for incoming serial messages from Pi
  if (Serial.available()) {
    serialEvent();
  }
  // If Pi is not in sleeping mode, read the sensors every 200ms
  if(!pi_sleeping){
    if (millis() - sampleLastTime > 200) {
      readSensors();
      sampleLastTime = millis();
    }
  }
  // At every second when Pi sleep command is issued
  if (millis() - mLastTime > 1000) {
    if(pi_sleeping){
//      if (sleep_period > 44000000) sleep_period = 0;  // second protection agaist sleeping more than 12h
      // If the oled is still on
      if(!power_off_display){
        // Clear and update the countdown on the oled display
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print((sleep_period - (millis() - pi_sleep_start))/1000 );
        // If Pi power is still up, display a second countdown for the power down
        if(!pi_down){
          display.setCursor(0, 10);
          display.print((wait_before_sleep*1000 - (millis() - pi_sleep_start))/1000 );       
        }
        display.display();
      }
    }
    // If a power cycle was issued, decrease its counter
    if(power_cycling) power_cycle_count--;    
    mLastTime = millis();
  }
  // Call a few functions on every loop
  update_oled();
  pi_standby();
  pi_power_cycle();
  fan_monitoring();
  monitor_menu_changes();
}
