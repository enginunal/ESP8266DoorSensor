# ESP8266DoorSensor
Door status monitor with ESP-12E. Sends door open/closed status info to IFTTT webhook service and local MQTT broker(Raspberry Pi).
  
  

We have two topics on the MQTT broker(Raspberry Pi) : 
- commands   : Commands topic for remote management of ESP8266
- doorstatus : Door status topic 
    
<br>
<br>

MQTT Sub Sample Script:
- mosquitto_sub -d -u engin -P deneme -t doorstatus

MQTT Pub Samples:
- mosquitto_pub -d -u engin -P deneme -t commands -m "1checkdoor"
- mosquitto_pub -d -u engin -P deneme -t commands -m "1ledon"
- mosquitto_pub -d -u engin -P deneme -t commands -m "1ledoff"
- mosquitto_pub -d -u engin -P deneme -t doorstatus -m "OPEN"
 
<br>   
<br>
<br>
  
 crontab job:
 - @reboot sleep 60 && /usr/bin/python /home/pi/mqttmain.py >> /home/pi/mqttlog.text 2>&1
 
  
   
   
