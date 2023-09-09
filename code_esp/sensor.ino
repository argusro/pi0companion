// variable to indicate when charging voltage is available
bool have_sun = true;

// Function to deal with button 1 interruption
void ICACHE_RAM_ATTR switch_1() {

  ESP.wdtFeed();

  int reading = digitalRead(SW1);

  if (reading == LOW) {
    // Debounce routine
    if ((millis() - lastDebounceTime) > debounceDelay) {
      lastDebounceTime = millis();
      // If display is off, turn it on
      if(power_off_display and (pi_down or low_power_detected)) {
        display.ssd1306_command(SSD1306_DISPLAYON); 
        power_off_display = false;
      }
      // If menu is telecom operator page, this section confirms the new value
      else if(change_op and selector == 2){
        change_op = false;
        char new_op[1];
        itoa(op_id,new_op,10);
        // Serial.println(new_op);
        writeFile(LittleFS, "/telecom.txt", new_op);
        uc2pi("telecom",operators[op_id]);
        change_time = millis();
        confirm_change = "op";
      }
      // If any other menu page, button 1 increase page
      else{
        if(selector == 0){
          page++;
          if(page > total_pages-1) page=0;
        }
        // In case of any of the confirmations bellow, button 1 confirms the option. 
        else if(ask_reboot or ask_shutdown or power_cycle) confirm = true;
        // In any other case, button 1 alters the value
        else toogle = true;
      }
    }
  }
}

// Function to deal with button 2 interruption
void ICACHE_RAM_ATTR switch_2() {

  ESP.wdtFeed();

  int reading = digitalRead(SW2);

  if (reading == HIGH) {
    // Debounce routine
    if ((millis() - lastDebounceTime2) > debounceDelay) {
      lastDebounceTime2 = millis();
      // When display is on
      if(!power_off_display) {
        // If Pi is supposed to be off, button 2 turn off display
        if(pi_down){
          display.ssd1306_command(SSD1306_DISPLAYOFF); 
          power_off_display = true;
        }
        else if(low_power_detected){
          display.ssd1306_command(SSD1306_DISPLAYOFF); 
          power_off_display = true;
        }
        else{
          // In the menu, button 2 will cancel the confirmations below
          if(ask_reboot or ask_shutdown or power_cycle) {
            page = 3;
            selector = 0;
            ask_reboot = false;
            ask_shutdown = false;
            power_cycle = false;
          }
          // In any other case, button 2 will change se selection in the same page
          else {
            selector++;
            if(selector > page_options-1) selector=0;
          }
        }
      }
    }
  }
}

// // Sensor variables are saved in an array, this function return the average of this array
// float average (int * array, int len)  // assuming array is int.
// {
//   long sum = 0L ;  // sum will be larger than an item, long for safety.
//   for (int i = 0 ; i < len ; i++)
//     sum += array [i] ;
//   return  ((float) sum) / len ;  // average will be fractional, so float may be appropriate.
// }

// Read all sensors with this function
void readSensors() {
  // Shift arrays of samples
  for (int sample = 0; sample < samples-1; sample++){
    hum[sample]=hum[sample + 1];
    temp[sample]=temp[sample + 1];
    current_mA[sample]=current_mA[sample + 1];
    power_mW[sample]=power_mW[sample + 1];
    loadvoltage[sample]=loadvoltage[sample + 1];
    solar_v[sample]=solar_v[sample + 1];
  }
  // Add new sample to the last position
  hum[samples-1] = sensor.getRH();
  temp[samples-1] = sensor.getTemp();  
  current_mA[samples-1] = ina219.getCurrent_mA();
  power_mW[samples-1] = ina219.getPower_mW();
  busvoltage = ina219.getBusVoltage_V();
  shuntvoltage = ina219.getShuntVoltage_mV();
  loadvoltage[samples-1] = busvoltage + (shuntvoltage / 1000);

  float Analog0 = analogRead(A0);
  solar_v[samples-1] = (Analog0 / 1024) * 27;
  // Zero any noise from solar input voltage
  if (solar_v[samples-1] < 3)
    solar_v[samples-1] = 0;
  float solar_delta = abs(solar_v[samples-1] - solar_v[samples-2])/solar_v[samples-1];
  // Reset the valid samples used to make the avarage in case of subtle change in solar input.
  if(!isnan(solar_delta)) {
    if ( solar_delta > 0.1){
      valid_samples = 1;
    }
  }
  // // Reset the valid samples used to make the avarage in case of subtle change in power measurement
  // float power_delta = abs(power_mW[samples-1] - power_mW[samples-2])/power_mW[samples-1];  
  // if ( power_delta > 0.2){
  //   valid_samples = 1;
  // }
  
  // Reset average values
  hum_avg = 0;
  temp_avg = 0;
  current_mA_avg = 0;
  power_mW_avg = 0;
  loadvoltage_avg = 0;
  solar_v_avg = 0;

  // Sum all samples from each array  
  for (int sample = samples-1; sample >= samples-valid_samples; sample--){
    hum_avg += hum[sample];
    temp_avg += temp[sample];
    current_mA_avg += current_mA[sample];
    power_mW_avg += power_mW[sample];
    loadvoltage_avg += loadvoltage[sample];
    solar_v_avg += solar_v[sample];
  }
  
  // Divide each sum by the number of valid samples
  hum_avg /= valid_samples;
  temp_avg /= valid_samples;
  current_mA_avg /= valid_samples;
  power_mW_avg /= valid_samples;
  loadvoltage_avg /= valid_samples;
  solar_v_avg /= valid_samples;

  // Check for low power, waiting for 10 consecutive measurements to confirm
  if ( loadvoltage_avg < min_voltage) {
    low_power_counter++;
    if(low_power_counter > 10) low_power();
  }
  else low_power_counter=0;
 
  // Increase the number of samples used in the average for fast update on power on and subtle changes
  if(valid_samples < samples) valid_samples++;
}

// Function to serialize sensor values in a json structure
void sendSensors() {
  readSensors();
  StaticJsonDocument<400> doc;
  doc["Temperature"] = String(temp_avg, 2);
  doc["Humidity"] = String(hum_avg, 2);
  doc["Voltage"] = String(loadvoltage_avg, 2);
  doc["Current"] = String(current_mA_avg, 2);
  doc["Power"] = String(power_mW_avg, 0);
  doc["Solar"] = String(solar_v_avg, 2);
  doc["Uptime"] = String(millis() / 1000);

  serializeJson(doc, Serial);
//  serializeJsonPretty(doc, Serial);
  Serial.println();

}

// Function to read all I2C sensors corrected, to be used on troubleshooting
void scan_i2c() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");
  //  Serial.println(millis()/1000);

  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error == 4)
    {
      //      Serial.print("Unknown error at address 0x");
      Serial.print(".");
      //      if (address < 16)
      //        Serial.print("0");
      //      Serial.println(address, HEX);
    }
  }
  Serial.println("");
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");

  else
    Serial.println("done");
  delay(1000);           // wait 1 seconds for next scan
}

// // Clean variable arrays
// void reset_values(int until){
//   memset(hum, 0 , until);
//   memset(temp, 0 , until);
//   memset(current_mA, 0 , until);
//   memset(loadvoltage, 0 , until);
//   memset(power_mW, 0 , until);
//   memset(solar_v, 0 , until);
//   valid_samples = 1;
// }
