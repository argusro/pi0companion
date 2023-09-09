// Variable to store serial input message
static StaticJsonDocument<256> inputjson;

// Send variable as json to Pi
template <typename S,typename T> 
void uc2pi(S key, T value ){
  StaticJsonDocument<100> doc;
  doc[key] = value;
  serializeJson(doc, Serial);
  Serial.println();
}
// Sendo multiple key/value json
void uc2pi_multi(String key, int value, String key2, int value2, String key3, int value3){
  StaticJsonDocument<300> doc;
  doc[key] = value;
  doc[key2] = value2;
  doc[key3] = value3;
  serializeJson(doc, Serial);
  Serial.println();
}
// Action on messages received from Pi
void deal_with_json(){
// Receive
//{"boot":"Lap00"}
//{"data":"telemetry"}
//{"data":"i2c"}
//{"fan":0}
//{"min_voltage":9}
//{"device":"Lap00"}
//{"sleep":60}
//{"going_down":true}
//{"setup_tl":{"start":6,"end":10,"interval":15}}
//{"setup_mgmt":{"wifi":true,"stream":true}}
//{"deep":1}
//{"watchdog":false}
//{"i2c_clean":true}
//{"confirm":"wifi/stream/tl/op"}
//{"i2c_power":true}/false}

  // Update time of received message
  last_comm = millis();
  // When Pi serial service start it should send the hostname
  if(inputjson.containsKey("boot")){
    String data_value = inputjson["boot"];
    if ( data_value.indexOf(DEVICE) >= 0 ) {
      rebooting = false;
      shutting_down = false;
      power_cycling = false;
      page = 0;
      selector = 0;
      // If match send it back the firmware version
      uc2pi("message",DEVICE + " version " + Firmware);
      // if uC is sleeping, time to wake up
      if(pi_sleeping) wakeup = true;
    }
    else{
      // Update Device name if differ from stored locally 
      const char* name_char = inputjson["boot"];//.c_str();
      writeFile(LittleFS, "/Device.txt", name_char);
      uc2pi("message","New device name: " + String(name_char));
      delay(2000);
      ESP.restart();
    }
  }
  
  // Return request for sensor data
  if(inputjson.containsKey("data")){
    String data_value = inputjson["data"];
    if ( data_value.indexOf("telemetry") >= 0 ) sendSensors();
    if ( data_value.indexOf("i2c") >= 0 ) scan_i2c();
  }
  
  // Act on commands for the fan
  if(inputjson.containsKey("fan")){
    int fan_speed = inputjson["fan"];  
    uc2pi("fan",fan_speed);
    if(fan_speed > 0) fan_speed = map(fan_speed, 1, 100, min_fan_speed, 1023);
    analogWrite(fan_in, fan_speed);
  }

  // Option to alter minimum voltage for troubleshooting
  if(inputjson.containsKey("min_voltage")){
    min_voltage = inputjson["min_voltage"];
    String msg = "Set minimum voltage to be monitored to " + String(min_voltage);
    uc2pi("message",msg);
  }
  
  // Store new devide name
  if(inputjson.containsKey("device")){
    const char* name_char = inputjson["device"];//.c_str();
    writeFile(LittleFS, "/Device.txt", name_char);
    uc2pi("message","New device name: " + String(name_char));
    delay(2000);
    ESP.restart();
  }
 
  // Tell uC to power down Pi for the desired duration
  if(inputjson.containsKey("sleep")){
    count_to_wakeup = inputjson["sleep"];
    if ( count_to_wakeup > 64800 or count_to_wakeup < 0) count_to_wakeup = 60;          // protect agains orders of sleeping more than 18h or negative values
    sleep_period = count_to_wakeup * 1000;
    pi_sleeping = true;
    analogWrite(fan_in, 0);
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Going to sleep for");
    display.setCursor(0, 10);
    display.print(count_to_wakeup);
    display.print(" seconds");
    display.display();
    String msg = "Wakeup in " + String(count_to_wakeup) + " seconds, bye!";
    uc2pi("message",msg);
    delay(1000);
    // count_to_wakeup *= 60;
    count_to_wakeup -= (wait_before_sleep + 4);
    pi_sleep_start = millis();
    valid_samples = 1;
  }

  // Print message on display for when Pi is shutting down
  if(inputjson.containsKey("going_down")){
    uc2pi("message","Pi going down received");
    analogWrite(fan_in, 0);
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Shutdown initiated");
    display.display();
    delay(2000);
    for(int count_down = 15; count_down > 0; count_down--){
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Wait");
      display.setCursor(0, 10);
      display.print(count_down);
      display.display();
      delay(1000);
      ESP.wdtFeed();
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("You can turn off now");
    display.setCursor(0, 12);
    display.print("Bye!");
    display.display();
    long wait_time = millis();
    while (millis()-wait_time < 30000) {
      ESP.wdtFeed();
      delay(5000);
    }
  }

  // Specific for camera project, update menu settings
  if(inputjson.containsKey("setup_tl")){
    tl_start = inputjson["setup_tl"]["start"];
    tl_end = inputjson["setup_tl"]["end"];
    int tl_frequency = inputjson["setup_tl"]["interval"];
    for (int idx=0; idx<tl_freq_opt;idx++){
      if (tl_frequency == tl_freq_list[idx]){
        tl_freq_index = idx;
      }
    }
    saved_tl_start = tl_start;
    saved_tl_end = tl_end;
    saved_tl_freq_index = tl_freq_index;
    String msg = "Timelapse setup set by Pi, start:" + String(tl_start) + "h, end:" + String(tl_end) + "h, interval:"+ String(tl_frequency) + "min";
    uc2pi("message",msg);
  }

  // Specific for camera project, update menu settings
  if(inputjson.containsKey("setup_mgmt")){
    wifi = inputjson["setup_mgmt"]["wifi"];
    stream = inputjson["setup_mgmt"]["stream"];
    String msg = "Management setup set by Pi, wifi:" + String(wifi) + " ,streaming:" + String(stream);
    uc2pi("message",msg);
  }
 
  // Call deep sleep for the uC - currently not working, since GPIO doesn't hold and Pi is powed on again
  if(inputjson.containsKey("deep")){
    int deel_sleep = inputjson["deep"];
    display.clearDisplay();
    display.display();
    pi_power(false);
    delay(5000);
    ESP.deepSleep(deel_sleep * 60e6);
  }

  // Enable/Disable waching for Pi communication
  if(inputjson.containsKey("watchdog")){
    watch_for_pi = inputjson["watchdog"];
    String msg = "uC as watchdog: " + String(watch_for_pi);
    uc2pi("message",msg);
  }

  // Return that menu changes sent were received by Pi
  if(inputjson.containsKey("confirm")){
    String confirm_serial = inputjson["confirm"];
    if ( confirm_serial == confirm_change ){
      confirm_change = "";
    }
  }

  // Option to reset I2C bus from Pi
  if(inputjson.containsKey("i2c_clean")){
    i2c_power(false);
    I2C_ClearBus();
    uc2pi("message","I2C cleaned");
    delay(1000);
    ESP.restart();
  }

  // Power down I2C bus from Pi
  if(inputjson.containsKey("i2c_power")){
    bool i2c_power_flag = inputjson["i2c_power"];
    i2c_power(i2c_power_flag);
    uc2pi("message","I2C set to: "+ String(i2c_power_flag));
  }
}

// Treat serial event
void serialEvent(){                                              
  while (Serial.available())
  {
    ESP.wdtFeed();
    const auto deser_err = deserializeJson(inputjson, Serial);
    if (deser_err) uc2pi("error","Failed to deserialize JSON");
//    else  serializeJson(inputjson, Serial); 
  }
  deal_with_json();
}
