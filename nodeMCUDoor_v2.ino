#include <ESP8266WiFi.h>  // Enables the ESP8266 to connect to the local network (via WiFi)
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <PubSubClient.h> // Connect and publish to the MQTT broker
#include "Pushover.h"


//Door Sensor
#define DOOR_SENSOR_PIN 4

String doorStatus = "";

//LED on/off
bool LED_ON = false;

// WiFi config
const char* ssid = "*****CVD";
const char* wifi_password = "******";

//IFTTT config
const char* ifttt_resource = "/trigger/DOOR/with/key/*********";
const char* ifttt_server = "maker.ifttt.com";


// MQTT config
const char* mqtt_server = "192.168.1.102";  // IP of the MQTT broker
const char* door_topic = "doorstatus";
const char* commands_topic = "commands";
const char* mqtt_username = "engin"; // MQTT username
const char* mqtt_password = "EnginUnal"; // MQTT password
const char* clientID = "ESPDoor"; // MQTT clientID

bool MQTT_IS_ON = true;
bool MQTT_PUBLISH = true;


//Static IP config
IPAddress local_IP(192, 168, 1, 76);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional


//Initialise the WiFi and MQTT Client 
WiFiClient wifiClient;
//1883 is the listener port for the MQTT Broker
PubSubClient mqttClient(mqtt_server, 1883, wifiClient);



// Establish a Wi-Fi connection with your router
void initWifi()
{
  delay(10);

  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
    Serial.println("STA Failed to configure");
  }

  Serial.print("Connecting to: ");
  Serial.println(ssid);

  // Connect to the WiFi
  WiFi.begin(ssid, wifi_password);

  int timeout = 10 * 4; // 10 seconds
  while (WiFi.status() != WL_CONNECTED && (timeout-- > 0))
  {
    delay(250);
    Serial.print(".");
  }

  Serial.println("");

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Failed to connect");
  }

  //Debugging - Output the IP Address of the ESP8266
  Serial.print("WiFi connected in: ");
  Serial.print(millis());
  Serial.print(", IP address: ");
  Serial.println(WiFi.localIP());
}

//Connect to the MQTT broker via WiFi
bool reconnect2MQTT()
{
  int retries = 10;

  while (!mqttClient.connected() && (retries-- > 0))
  {
    Serial.print("Attempting MQTT connection...");
    /**** Attempt to connect ***** 
    * If You're Having Problems With Mqtt Multiple Connections, To change the ESP device ID, 
    * you will have to give a unique name to the ESP8266. That should solve your MQTT multiple connections problem
    * ex: mqttClient.connect("ESP8266ClientDoor") or mqttClient.connect("ESP8266ClientRoom") or ...  
    * !!!! If the connection is failing, make sure you are using the correct MQTT Username and Password !!!!
    */
    if (mqttClient.connect(clientID, mqtt_username, mqtt_password))
    {
      delay(10); // This delay ensures that mqttClient.publish doesn't clash with the mqttClient.connect call
      Serial.println("connected");
      //Once connected, publish an announcement...
      mqttClient.publish(door_topic, "HELLO from ESP8266!");
      // Subscribe or resubscribe to a topic, you can subscribe to more topics 
      mqttClient.subscribe(commands_topic);
      return true;
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println("trying again...");
      delay(500);
    }

  }//while

  return false;
}

//This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }

  Serial.println();

  //commands
  if (messageTemp == "1restart")
  { //reset
    Serial.print("ESP is restarting...");
    ESP.restart();
  }
  else if (messageTemp == "1stop")
  { //stop
    Serial.print("wifi client is stopping..");
    wifiClient.stop();
  }
  else if (messageTemp == "1deepsleep")
  { //deepsleep
    Serial.print("ESP is going to deepsleep");
    // Deep sleep mode until RESET pin is connected to a LOW signal 
    ESP.deepSleep(0);
  }
  else if (messageTemp == "1ledon")
  { //ledon
    Serial.print("led is on");
    LED_ON = true;
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on by making the voltage LOW
  }
  else if (messageTemp == "1ledoff")
  { //ledoff
    Serial.print("led is off");
    LED_ON = false;
    digitalWrite(BUILTIN_LED, HIGH);   // Turn the LED off (the built-in led on a Node MCU board is active low)
  }
  else if (messageTemp == "1checkdoor")
  { //checkDoorState
    Serial.print("checking door state");
    checkDoorState();
  }
  else if (messageTemp == "1mqttoff")
  { 
    Serial.print("mqtt is off");
    MQTT_IS_ON = false;
  }
  else if (messageTemp == "1mqttpublishon")
  { 
    Serial.print("mqtt publishing is on");
    MQTT_PUBLISH = true;
  }
  else if (messageTemp == "1mqttpublishoff")
  { 
    Serial.print("mqtt publishing is off");
    MQTT_PUBLISH = false;
  }


  Serial.println();
}

void publishMQTTMessage(String topic, String message)
{
  if (!mqttClient.connected())
    reconnect2MQTT();

  //PUBLISH to the MQTT Broker 
  if (mqttClient.publish(topic.c_str(), message.c_str()))
  {
    Serial.println("MQTT message sent!");
  }
  else
  {
    Serial.println("MQTT message failed to send. Reconnecting to MQTT Broker and trying again");
    reconnect2MQTT();
    delay(10); // This delay ensures that mqttClient.publish doesn't clash with the mqttClient.connect call
    mqttClient.publish(topic.c_str(), message.c_str());
  }
}

// Make an HTTP request to the IFTTT web service
void IFTTTgetReq()
{
  //Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi Disconnected");
    return;
  }

  Serial.print("Connecting to ");
  Serial.print(ifttt_server);

  int retries = 5;
  while (!wifiClient.connect(ifttt_server, 80) && (retries-- > 0))
  {
    delay(100);
    Serial.print(".");
  }

  Serial.println();

  if (!wifiClient.connected())
  {
    Serial.println("Failed to connect, going back to sleep");
  }

  Serial.print("Request ifttt_resource: ");
  Serial.println(ifttt_resource);
  wifiClient.print(String("GET ") + ifttt_resource +
                  " HTTP/1.1\r\n" +
                  "Host: " + ifttt_server + "\r\n" +
                  "Connection: close\r\n\r\n");

  int timeout = 5 * 10; // 5 seconds             
  while (!wifiClient.available() && (timeout-- > 0))
  {
    delay(100);
  }

  if (!wifiClient.available())
  {
    Serial.println("No response\n");
  }

  while (wifiClient.available())
  {
    Serial.write(wifiClient.read());
  }

  Serial.println("Closing connection\n");
  wifiClient.stop();
}

void IFTTTpostReq(String value1, String value2)
{
  String httpAddr = "http://";
  httpAddr += ifttt_server;
  httpAddr += ifttt_resource;
  
  Serial.print("Connecting to ");
  Serial.print(httpAddr);

  //Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi Disconnected");
    return;
  }

  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(httpAddr);

  // Specify content-type header
  http.addHeader("Content-Type", "application/json");
  // JSON data to send with HTTP POST   {"value1":"OPEN"} or {"value1":"CLOSED"} 
  String httpRequestData = "{\"value1\":\"" + value1 + "\",\"value2\":\"" + value2 + "\"}";
  // Send HTTP POST request
  int httpResponseCode = http.POST(httpRequestData); 

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  // Free resources
  http.end();
}

// Make an HTTP request to the Pushover web service
void PushOverPostReq(String msg)
{
  //Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi Disconnected");
    return;
  }

  Pushover po = Pushover("*****","********", UNSAFE);
  po.setMessage(msg);
  Serial.println(po.send()); //should return 1 on success
}

ICACHE_RAM_ATTR void doorChanged()
{
  if (digitalRead(DOOR_SENSOR_PIN) == HIGH)
  {
    Serial.println("The door got closed");
    doorStatus = "CLOSED"; 
  }
  else
  {
    Serial.println("The door got opened!");
    doorStatus = "OPEN";
  }
}

void checkDoorState()
{
  if (digitalRead(DOOR_SENSOR_PIN) == HIGH)
  {
    Serial.println("The door got closed");
    //MQTT 
    if (MQTT_PUBLISH)
      publishMQTTMessage(door_topic, "CLOSE");

    //IFTT 
    IFTTTpostReq("CLOSE", "");
  }
  else
  {
    Serial.println("The door got opened!");
    //MQTT 
    if (MQTT_PUBLISH)
      publishMQTTMessage(door_topic, "OPEN");

    //IFTT 
    IFTTTpostReq("OPEN", "");
  }
}

void setup()
{
  //PINS
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
  
  //INTERRUPT
  attachInterrupt(digitalPinToInterrupt(DOOR_SENSOR_PIN), doorChanged, CHANGE);
  
  Serial.begin(115200);
  
  //WIFI
  initWifi();
  
  //MQTT
  if (MQTT_IS_ON)
  {
    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(callback);
  }

  digitalWrite(BUILTIN_LED, HIGH);   // Turn the LED off 
}

void loop()
{
  //MQTT
  if (MQTT_IS_ON)
  {
    if (!mqttClient.connected())
      reconnect2MQTT();
  
    mqttClient.loop();
  }
  
  if (doorStatus != "")
  {
    if (!LED_ON)
      digitalWrite(BUILTIN_LED, LOW);
    
    //MQTT 
    if (MQTT_PUBLISH)
      publishMQTTMessage(door_topic, doorStatus);
    
    //IFTT 
    IFTTTpostReq(doorStatus, "");
    PushOverPostReq(doorStatus);
    //IFTTTgetReq();
    
    doorStatus = "";

    if (!LED_ON)
      digitalWrite(BUILTIN_LED, HIGH);
  }


  delay(100);
}
