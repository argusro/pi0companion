// Flag to register if Pi WatchDog should be in operation
bool watch_for_pi = true;

// Function to enable or disable Pi power supply depenting on board version
void pi_power(bool en){
  if(board_version == 0.0){
    if(en) digitalWrite(soc_pw_pin, LOW );
    else digitalWrite(soc_pw_pin, HIGH );
  }
  else{
    if(en) digitalWrite(soc_pw_pin, HIGH );
    else digitalWrite(soc_pw_pin, LOW );
  }
}

// Function to manage sensors power
void i2c_power(bool en){
  if(en) digitalWrite(i2c_gnd, HIGH);
  else digitalWrite(i2c_gnd, LOW);
}

// Check Pi power situation
void pi_standby() {
  // If it should be sleeping
  if (pi_sleeping) { 
    // Disable oled display after wait period is over
    if (millis() - pi_sleep_start  >= wait_before_sleep*1000 and !pi_down) {
      pi_power(false);
      pi_down = true;
      display.clearDisplay();
      display.display();
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      power_off_display = true;
    }
    // Turn display on again when period is over
    if (millis() - pi_sleep_start  >= sleep_period or wakeup){
      display.ssd1306_command(SSD1306_DISPLAYON);
      power_off_display = false;
      pi_power(true);
      ESP.wdtFeed();
      delay(1000);
      pi_sleeping = false;
      pi_down = false;
      valid_samples = 1;
      wakeup = false;
    }
  }
  // If it should not be sleeping
  else{
    // If Pi WatchDog is on
    if (watch_for_pi) pi_alive();
    // If a day has passed, reset uC
    if (millis() > 86400000 ) {
      uc2pi("warning","uC up for 1 day, rebooting");
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("ESP up for 1 day");
      display.setCursor(0, 10);
      display.print("rebooting");
      display.display();
      delay(2000);
      ESP.restart();
    }
  }
}

// Check if Pi as communicated in the past 11 min, otherwise powercycle, unless it is supposed to be sleeping
void pi_alive(){
  if ((millis() - last_comm > 660000) and !pi_sleeping ){
    ESP.wdtFeed();
    last_comm = millis();
    uc2pi("warning","Pi missing, forcing reboot");
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Missing Pi");
    display.setCursor(0, 10);
    display.print("rebooting");
    display.display();
    pi_power(false);
    delay(1000);
    pi_power(true);      
  }
}

// If a low power was confirmed, turn off Pi and wait 10 min to check again
void low_power(){
  ESP.wdtFeed();
  low_power_detected = true;
  power_off_display = true;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("LOW POWER!!");
  display.setCursor(0, 15);
  display.print("Going to Sleep");
  display.display();
  delay(3000);
  pi_power(false);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Low Battery");
  display.display();
  delay(3000);
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  low_power_sleep = millis();
  while(millis()-low_power_sleep < 600000){
    ESP.wdtFeed();
    delay(5000);
  }
  ESP.restart();
  //ESP.deepSleep(300e6); 
}

// If a powercycle is called, wait 20 seconds and execute.
void pi_power_cycle(){
  if(power_cycling and (millis()- power_cycle_waiting) > 20000){
    pi_power(false);
    delay(4000);
    power_cycling = false;
    page = 0;
    selector = 0;
    pi_power(true);   
  }
}

// Check temperature to trigger fan if necessary
void fan_monitoring(){
  if(!pi_sleeping and solar_v_avg > 10){
    if(temp_avg > 50 and !ckt_hot){
      int fan_speed = map(temp_avg, 50, 70, min_fan_speed, 1023);
      uc2pi("fan",int(100*fan_speed/1023));
      analogWrite(fan_in, fan_speed);
      ckt_hot = true;  
    }
    if(temp_avg < 45 and ckt_hot){
      analogWrite(fan_in, 0);
      uc2pi("fan",0);
      ckt_hot = false;
    }
  }
  else {
    if(ckt_hot){
      analogWrite(fan_in, 0);
      uc2pi("fan",0);
      ckt_hot = false;
    }
  }
}

// Wait for confirmation from Pi when menu configuration was changed locally, otherwise rever change.
void monitor_menu_changes(){
  if (confirm_change.length() > 0){
    // Wait for 30 seconds
    if (millis()-change_time > 30000){
      String to_rever = confirm_change;
      if (to_rever == "wifi"){
        wifi = !wifi;
      }
      if (to_rever == "stream"){
        stream = !stream;
      }
      if (to_rever == "tl"){
        tl_start = saved_tl_start;
        tl_end = saved_tl_end;
        tl_freq_index = saved_tl_freq_index;
      }
      if (to_rever == "op"){
        op_id = saved_op;
        char new_op[1];
        itoa(op_id,new_op,10);
        writeFile(LittleFS, "/telecom.txt", new_op);
      }
      confirm_change = "";
    }
  }
}

// Sometimes the I2C Bus get stuck, this function will reset and get back working.
int I2C_ClearBus() {
  #if defined(TWCR) && defined(TWEN)
    TWCR &= ~(_BV(TWEN)); //Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
  #endif
    pinMode(SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
    pinMode(SCL, INPUT_PULLUP);

    delay(2500);  // Wait 2.5 secs. This is strictly only necessary on the first power
    // up of the DS3231 module to allow it to initialize properly,
    // but is also assists in reliable programming of FioV3 boards as it gives the
    // IDE a chance to start uploaded the program
    // before existing sketch confuses the IDE by sending Serial data.

    boolean SCL_LOW = (digitalRead(SCL) == LOW); // Check is SCL is Low.
    if (SCL_LOW) { //If it is held low Arduno cannot become the I2C master. 
      return 1; //I2C bus error. Could not clear SCL clock line held low
    }

    boolean SDA_LOW = (digitalRead(SDA) == LOW);  // vi. Check SDA input.
    int clockCount = 20; // > 2x9 clock

    while (SDA_LOW && (clockCount > 0)) { //  vii. If SDA is Low,
      clockCount--;
    // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
      pinMode(SCL, INPUT); // release SCL pullup so that when made output it will be LOW
      pinMode(SCL, OUTPUT); // then clock SCL Low
      delayMicroseconds(10); //  for >5uS
      pinMode(SCL, INPUT); // release SCL LOW
      pinMode(SCL, INPUT_PULLUP); // turn on pullup resistors again
      // do not force high as slave may be holding it low for clock stretching.
      delayMicroseconds(10); //  for >5uS
      // The >5uS is so that even the slowest I2C devices are handled.
      SCL_LOW = (digitalRead(SCL) == LOW); // Check if SCL is Low.
      int counter = 20;
      while (SCL_LOW && (counter > 0)) {  //  loop waiting for SCL to become High only wait 2sec.
        counter--;
        delay(100);
        SCL_LOW = (digitalRead(SCL) == LOW);
      }
      if (SCL_LOW) { // still low after 2 sec error
        return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
      }
      SDA_LOW = (digitalRead(SDA) == LOW); //   and check SDA input again and loop
    }
    if (SDA_LOW) { // still low
      return 3; // I2C bus error. Could not clear. SDA data line held low
    }

    // else pull SDA line low for Start or Repeated Start
    pinMode(SDA, INPUT); // remove pullup.
    pinMode(SDA, OUTPUT);  // and then make it LOW i.e. send an I2C Start or Repeated start control.
    // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
    /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
    delayMicroseconds(10); // wait >5uS
    pinMode(SDA, INPUT); // remove output low
    pinMode(SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
    delayMicroseconds(10); // x. wait >5uS
    pinMode(SDA, INPUT); // and reset pins as tri-state inputs which is the default state on reset
    pinMode(SCL, INPUT);
    return 0; // all ok
  }

