// Menu setup
#define total_pages 5
#define back_to_page0 60

int page = 0;
bool wifi = false;
bool stream = false;
int selector = 0;
int page_options = 1;
bool toogle = false;
int tl_start = 6;
int tl_end = 18;
int tl_freq_list[] = {5,10,15,20,30,60,1,2};
int tl_freq_index = 0;
int saved_tl_start = 0;
int saved_tl_end = 0;
int saved_tl_freq_index = 0;
int saved_op = 0;
int tl_freq_opt = sizeof(tl_freq_list) / sizeof(tl_freq_list[0]);
bool tl_changed = false;
bool ask_reboot = false;
bool ask_shutdown = false;
bool power_cycle = false;
bool confirm = false;
bool rebooting = false;
bool shutting_down = false;
bool change_op = false;

// Called by main loop
void update_oled() {
  if(!pi_sleeping){
    if(page!=0){  // after 10 min of no changes go back to main screen
      if(millis()-lastDebounceTime > 600000 and millis()-lastDebounceTime2 > 600000) {
        page = 0;
        selector = 0;
      }
    }
    // Main page shows power and sensor information
    if(page==0){
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("V:");
      display.print(loadvoltage_avg, 2);
      display.print(" v");
      display.setCursor(80, 0);
      display.print(temp_avg, 0);
      display.print(" ");
      display.print((char)247);
      display.print("C");
      display.setCursor(0, 10);
      display.print("I:");
      display.print(current_mA_avg, 1);
      display.print(" mA");
      display.setCursor(80, 10);
      display.print(hum_avg, 0);
      display.print(" %");
      display.setCursor(0, 20);
      display.print("P:");
      display.print(power_mW_avg, 0);
      display.print(" mW");
      display.setCursor(75, 20);
      if (solar_v_avg ==0){
        display.print("No Sun");
      }
      else {
        display.print("S:");
        display.print(solar_v_avg, 1);
        display.print(" v");
      }
      display.display();
    }
    // Page 1 used for camera project, enable local wifi or camera streaming
    else if (page == 1){      // wifi/stream configuration
      page_options = 3;
      display.clearDisplay();
      display.setCursor(20, 0);
      display.print("Config");
      display.setCursor(10, 12);
      display.print("Wifi AP: ");
      if(wifi) {
        // Flashes value until confirmed by Pi
        if (confirm_change == "wifi"){
          if ((int((millis() - change_time)/500) % 2) == 0){
            display.print("ON");
          }
        }
        else {
          display.print("ON"); 
          display.setCursor(10, 24);
          display.print("Stream: ");
          if (confirm_change == "stream"){
            if ((int((millis() - change_time)/500) % 2) == 0){
              if(stream) display.print("ON");
              else display.print("OFF");  
            }
          }  
          else {
            if(stream) display.print("ON");
            else display.print("OFF");  
          }   
          
        }     
      }
      else {
        if (confirm_change == "wifi"){
          if ((int((millis() - change_time)/500) % 2) == 0){
            display.print("OFF");
          }
        }
        else{
          display.print("OFF");
          if(stream){
            stream = false;
            uc2pi("stream", stream);
          }
          if (selector == 2) selector = 0;
          if(millis()-lastDebounceTime > back_to_page0 * 1000 and millis()-lastDebounceTime2 > back_to_page0 * 1000) {
            page = 0;
            selector = 0;
          }
        }
      }
      if (selector == 0){
        display.setCursor(5, 0);
        display.print(">");
      }
      else if (selector == 1){
        display.setCursor(0, 12);
        display.print(">");
        if(toogle) {
          wifi = !wifi;
          uc2pi("wifi",wifi);
          toogle = false;
          if (confirm_change.length() == 0){
            confirm_change = "wifi";
            change_time = millis();
          }
          else confirm_change = "";
        }
      }
      else if (selector == 2){
        display.setCursor(0, 24);
        display.print(">");
        if(toogle) {
          stream = !stream;
          uc2pi("stream", stream);
          toogle = false;
          if (confirm_change.length() == 0){
            confirm_change = "stream";
            change_time = millis();
          }
          else confirm_change = "";
        }
      }
      
      display.display();
    }
    // Page 2 used for camera project, configure cron job on Pi
    else if (page == 2){
      page_options = 5;
      display.clearDisplay();
      display.setCursor(20, 0);
      display.print("Timelapse");
      display.setCursor(10, 12);
      display.print("Start:");
      display.print(tl_start);
      display.setCursor(10, 24);
      display.print("End: ");
      display.print(tl_end);
      display.setCursor(80, 12);
      display.print("Freq:");
      display.print(tl_freq_list[tl_freq_index]);
      display.setCursor(80, 24);
      if (confirm_change == "tl"){
        if ((int((millis() - change_time)/500) % 2) == 0){
          display.print("Saving");
        }
      }
      else if(tl_changed){
         display.print("Save?");
      }
      if (selector == 0){
        display.setCursor(5, 0);
        display.print(">");
      }
      else if (selector == 1){
        display.setCursor(0, 12);
        display.print(">");
        if(toogle) {
          toogle = false;
          tl_start++;
          if (tl_start > 23) tl_start = 0;
          tl_changed = true;
        }
      }
      else if (selector == 2){
        display.setCursor(0, 24);
        display.print(">");
        if(toogle) {
          toogle = false;
          tl_end++;
          if(tl_end <= tl_start) tl_end = tl_start + 1;
          if (tl_end > 23) tl_end = tl_start + 1;
          tl_changed = true;
        }
      }
      else if (selector == 3){
        display.setCursor(70, 12);
        display.print(">");
        if(toogle) {
          toogle = false;
          tl_freq_index++;
          if (tl_freq_index >= tl_freq_opt) tl_freq_index = 0;
          tl_changed = true;
        }
      }
      else if (selector == 4){
        if(tl_changed){
          display.setCursor(70, 24);
          display.print(">");
          if(toogle) {
            toogle = false;
            uc2pi_multi("tl_start",tl_start,"tl_end",tl_end,"tl_frequency",tl_freq_list[tl_freq_index]);
            tl_changed = false;
            selector = 0;
            confirm_change = "tl";
            change_time = millis();
          }
        }
        else selector = 0;
      }
      display.display();
      if (!tl_changed){
        if(millis()-lastDebounceTime > back_to_page0 * 1000 and millis()-lastDebounceTime2 > back_to_page0 * 1000) {
          page = 0;
          selector = 0;
        }
      }
    }
    // Page 3 have power options for Pi
    else if (page == 3){
      page_options = 4;
      display.clearDisplay();
      display.setCursor(20, 0);
      display.print("Power Pi");
      display.setCursor(10, 12);
      display.print("Reboot");
      display.setCursor(10, 24);
      display.print("Shutdown");
      display.setCursor(80, 13);
      display.print("Power");
      display.setCursor(80, 23);
      display.print("Cycle");
      if (selector == 0){
        display.setCursor(5, 0);
        display.print(">");
      }
      else if (selector == 1){
        display.setCursor(0, 12);
        display.print(">");
        if(toogle) {
          toogle = false;
          ask_reboot = true;
        }
      }
      else if (selector == 2){
        display.setCursor(0, 24);
        display.print(">");
        if(toogle) {
          toogle = false;
          ask_shutdown = true;
        }
      }
      else if (selector == 3){
        display.setCursor(70, 18);
        display.print(">");
        if(toogle) {
          toogle = false;
          power_cycle = true;
        }
      }
      if(ask_reboot or ask_shutdown or power_cycle){
        display.clearDisplay();
        display.setCursor(30, 0);
        if(ask_reboot) display.print("  Reboot");
        if(ask_shutdown) display.print(" Shutdown");
        if(power_cycle) display.print("Power Cycle");
        display.setCursor(25, 12);
        display.print("Are you sure?");
        display.setCursor(10, 24);
        display.print("YES");
        display.setCursor(100, 24);
        display.print("NO");
        if(confirm) {
          confirm = false;
          if(ask_reboot) {
            ask_reboot = false;
            uc2pi("reboot", true);
            rebooting = true;
          }
          if(ask_shutdown) {
            ask_shutdown = false;
            uc2pi("shutdown", true);
            shutting_down = true;
          }
          if(power_cycle) {
            power_cycle = false;
            uc2pi("power_cycle", true);
            power_cycling = true;
            power_cycle_waiting = millis();
            power_cycle_count = 20;
          }
        }
      }
      else if(rebooting or shutting_down or power_cycling){
        display.clearDisplay();
        display.setCursor(20, 0);
        if (rebooting) display.print("Rebooting");
        if (shutting_down) display.print("Shutting Down");
        if (power_cycling) {
          display.print("Power Cycling Pi");
          display.setCursor(10, 20);
          display.print(power_cycle_count); 
        }
        display.setCursor(40, 20);
        display.print("Please wait!");
      }
      else {
        if(millis()-lastDebounceTime > back_to_page0 * 1000 and millis()-lastDebounceTime2 > back_to_page0 * 1000) {
          page = 0;
          selector = 0;
        }
      }
      display.display();
      
    }
    // Page 4 have option to change telecom operator
    else if (page == 4){
      page_options = 3;
      display.clearDisplay();
      display.setCursor(20, 0);
      display.print("Mobile");
      display.setCursor(10, 12);
      display.print("ISP:");
      display.setCursor(35, 12);
      display.print(operators[op_id]);
      display.setCursor(90, 24);
      if (confirm_change == "op"){
        if ((int((millis() - change_time)/500) % 2) == 0){
          display.print("Saving");
        }
      }
      else if(change_op){
        display.print("Save?");
      }
      else{
        if (selector == 2) selector = 0;
      }
    
      if (selector == 0){
        display.setCursor(5, 0);
        display.print(">");
      }
      else if (selector == 1){
        display.setCursor(0, 12);
        display.print(">");
        if(toogle) {
          op_id++;
          if(op_id == total_operators) op_id=0;
          change_op = true;
          toogle = false;
        }
      }
      else if (selector == 2){
        display.setCursor(80, 24);
        display.print(">");
      }
      display.display();
    }
  }
}
