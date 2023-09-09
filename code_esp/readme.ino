/*
  Features:
  v3.06
  -create single function to serialize json with template
  v3.05
  -disable fan when temp < 45
  v3.04
  -add TM as mobile operator
  v3.03
  -add i2c_gnd control to serial
  v3.01
  -fix fan active serial spam
  -revert board version
  -add revert values if not confirmed by Pi
  v2.06
  -cancel sleep if pi wakes up
  -i2c cleanup option
  v2.04
  -add telecom selection
  -adjusted max sleep to 18h
  v2.03
  -fix fan_speed=0
  V2.02
  -fan based on temp_avg
  -pi missing sent on serial
  v2.01
  -protect interval from negative values
  -rename pi_sleep_counter to pi_sleep_start
  v2.0
  -json on serial
  -oled menu
  -daily reboot
  -reboot pi if no comm in 11min
  -max sleep 12h
  -going down message
  v1.0
  -Oled Name
  v0.01
  -JSON serial
  -OLED
  -button 1 interrupt
  -send telemetry from serial data request
  -fans from seri


  Modules:
  INA219 - Power
  Si7021 - Weather
  SSD1306 - Oled 128x32
  
  Pinout
  OUT
  12 - FAN
  13 - 5V EN
  IN
  14 - SW1 ack on low
  15 - SW2 ack on high

  JSON - receive uptions and replies

{"boot":"Lap10"} - sent by pi on boot
if unchanged  {"request":"device_name"}
else          {"message":"Pi boot OK"}

{"data":"telemetry"} - request values to pub on thingsboard
              {"Temperature":"31.78","Humidity":"28.80","Voltage":"7.93","Current":"-230.24","Power":"1835","Solar":"0.00","Uptime":"154"}

{"data":"i2c"} - broken

{"fan":0-100} - set fan speed
              {"fan":0-100}

{"min_voltage":9} - for testing purpuse
              {"message":"Changed minimum voltage to be monitored to 9"}

{"device":"Lap08"} - apply+restart uC
`             {"message":"New device name: Lap00"}

{"sleep":60} - sleeps pi until the seconds are passed
              {"message":"Wakeup in 60 seconds, bye!"}

{"going_down":true} - just message on display for waiting to shutdown on switch
              {"message":"Pi going down received"}

{"setup_tl":{"start":6,"end":10,"interval":15}} - to update setup menu on display
              {"message":"Timelapse setup changed by Pi, start:6h ,end:10h ,interval:5min"}

{"setup_mgmt":{"wifi":true,"stream":true}} - to update setup menu on display
              {"message":"Management setup changed by Pi, wifi:1 , streaming:1"}

{"deep":1} - deep sleep test of 1 min, currently not working, pi wakes up

{"watchdog":false} - disable the 11 min monitor for connections if pi is not sleeping
              {"message":"uC as watchdog: 0"}

  JSON - send messages

{"boot":"uc"} - first message when uC reboots

{"error":"Failed to deserialize JSON"} - when the message received is not well formed

-menu set timelaspe
{"stream":true/false}
{"wifi":true/false}
{"tl_start":6,"tl_end":18,"tl_frequency":5}

{"reboot":true}
{"shutdown":true}
{"power_cycle":true}

*/
