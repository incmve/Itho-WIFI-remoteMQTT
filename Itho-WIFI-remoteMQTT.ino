// Includes
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <FS.h>
#include <SerialCommand.h>
#include "IthoCC1101.h"
#include <EEPROM.h>
#include <WiFiClient.h>
#include <TimeLib.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "config.h"


// WIFI
String ssid    = CONFIG_WIFI_SSID;
String password = CONFIG_WIFI_PASS;
String espName    = CONFIG_WIFI_NAME;



String ClientIP;

// webserver
ESP8266WebServer  server(80);
MDNSResponder   mdns;
WiFiClient client;

// DHT settings
const int DHTPIN = CONFIG_DHT_PIN;
#define DHTTYPE  CONFIG_DHT_TYPE   // DHT 22
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor
long DHTEnabled     = CONFIG_DHT_ENABLE; //1 is on
String laststate;
String CurrentState;
String CurrentHumidity;
String CurrentTemperature;

// MQTT
const char* mqttServer = CONFIG_MQTT_HOST;
const char* mqttUsername = CONFIG_MQTT_USER;
const char* mqttPassword = CONFIG_MQTT_PASS;
PubSubClient mqttclient(client);

// MQTT Topics
const char* temperatureTopic = CONFIG_MQTT_TOPIC_TEMP;
const char* humidityTopic = CONFIG_MQTT_TOPIC_HUMID;
const char* commandtopic = CONFIG_MQTT_TOPIC_COMMAND;
const char* responsetopic = CONFIG_MQTT_TOPIC_RESPONSE;

// Div
SerialCommand sCmd;
IthoCC1101 rf;
IthoPacket packet;
int timerx10;
unsigned long time2 = millis();


//HTML

String header       =  "<html lang='en'><head><title>Itho control panel</title><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><link rel='stylesheet' href='http://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js'></script><script src='http://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js'></script></head><body>";

String containerStart   =  "<div class='container'><div class='row'>";
String containerEnd     =  "<div class='clearfix visible-lg'></div></div></div>";
String siteEnd        =  "</body></html>";

String panelHeaderName    =  "<div class='col-md-4'><div class='page-header'><h1>";
String panelHeaderEnd   =  "</h1></div>";
String panelEnd       =  "</div>";

String panelBodySymbol    =  "<div class='panel panel-default'><div class='panel-body'><span class='glyphicon glyphicon-";
String panelBodyDuoSymbol    =  "</span></div><div class='panel-body'><span class='glyphicon glyphicon-";
String panelBodyName    =  "'></span> ";
String panelBodyValue   =  "<span class='pull-right'>";
String panelcenter   =  "<div class='row'><div class='span6' style='text-align:center'>";
String panelBodyEnd     =  "</span></div></div>";

String inputBodyStart   =  "<form action='' method='POST'><div class='panel panel-default'><div class='panel-body'>";
String inputBodyName    =  "<div class='form-group'><div class='input-group'><span class='input-group-addon' id='basic-addon1'>";
String inputBodyPOST    =  "</span><input type='text' name='";
String inputBodyClose   =  "' class='form-control' aria-describedby='basic-addon1'></div></div>";
String ithocontrol     =  "<a href='/button?action=Low'<button type='button' class='btn btn-default'> Low</button></a><a href='/button?action=Medium'<button type='button' class='btn btn-default'> Medium</button></a><a href='/button?action=High'<button type='button' class='btn btn-default'> High</button><a href='/button?action=Timer'<button type='button' class='btn btn-default'> Timer</button></a></a><br><a href='/button?action=Learn'<button type='button' class='btn btn-default'> Learn</button></a></div>";



// ROOT page
void handle_root()
{
  // get IP
  IPAddress ip = WiFi.localIP();
  ClientIP = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  delay(500);

  String title1     = panelHeaderName + String("Itho WIFI remote") + panelHeaderEnd;
  String IPAddClient    = panelBodySymbol + String("globe") + panelBodyName + String("IP Address") + panelBodyValue + ClientIP + panelBodyEnd;
  String ClientName   = panelBodySymbol + String("tag") + panelBodyName + String("Client Name") + panelBodyValue + espName + panelBodyEnd;
  String ithoVersion   = panelBodySymbol + String("ok") + panelBodyName + String("Version") + panelBodyValue + Version + panelBodyEnd;
  String State   = panelBodySymbol + String("info-sign") + panelBodyName + String("Current state") + panelBodyValue + CurrentState + panelBodyEnd;
  String DHTsensor   = panelBodySymbol + String("fire") + panelBodyName + String("Temperature") + panelBodyValue + CurrentTemperature + String(" °C") + panelBodyDuoSymbol + String("tint") + panelBodyName + String("Humidity") + panelBodyValue + CurrentHumidity + String(" %") + panelBodyEnd;
  String Uptime     = panelBodySymbol + String("time") + panelBodyName + String("Uptime") + panelBodyValue + hour() + String(" h ") + minute() + String(" min ") + second() + String(" sec") + panelBodyEnd + panelEnd;


  String title3 = panelHeaderName + String("Commands") + panelHeaderEnd;

  String commands = panelBodySymbol + panelBodyName + panelcenter + ithocontrol + panelBodyEnd;


  server.send ( 200, "text/html", header + navbar + containerStart + title1 + IPAddClient + ClientName + ithoVersion + State + DHTsensor + Uptime + title3 + commands + containerEnd + siteEnd);
}

String eepromRead(int StartAddress, int EepromLength)
{

  String Eeprom_Content;
  for (int i = StartAddress; i < (StartAddress + EepromLength); ++i)
  {
    if (EEPROM.read(i) != 0 && EEPROM.read(i) != 255)
    {
      Eeprom_Content += char(EEPROM.read(i));
    }
  }
  return Eeprom_Content;

}

void eepromWrite(int StartAddress, int EepromLength, String value)
{
  //Clear EEPROM first
  for (int i = StartAddress; i < (StartAddress + EepromLength); ++i)
  {
    EEPROM.write(i, 0);
  }
  int bytesCnt = StartAddress;
  for (int i = 0; i < value.length(); ++i)
  {
    EEPROM.write(bytesCnt, value[i]);
    ++bytesCnt;
  }

  // Commit changes to EEPROM
  EEPROM.commit();
  Serial.println("EEPROM saved");
  CurrentState = value;
}

// Setup
void setup(void)
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  EEPROM.begin(512); // On esp8266, we need to init EEPROM
  String laststate = eepromRead(0, 6);
  CurrentState = laststate;
  Serial.println();
  Serial.print("laststate: ");
  Serial.println(laststate);
  Serial.print("Timer state: : ");
  Serial.println(timerx10);
  WiFi.hostname(espName);

  WiFi.begin(ssid.c_str(), password.c_str());
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 31)
  {
    delay(1000);
    Serial.print(".");
    ++i;
  }
  if (WiFi.status() != WL_CONNECTED && i >= 30)
  {
    WiFi.disconnect();
    delay(1000);
    Serial.println("");
    Serial.println("Couldn't connect to network :( ");
    Serial.println("Review your WIFI settings");

  }
  else
  {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(espName);

  }
  delay(500);
  Serial.println("setup begin");
  rf.init();
  Serial.println("setup done");
  //sendRegister(); // Your Itho only accepts this command while in learning mode, powercycle your itho then start the nodemcu.
  Serial.println("join command sent");
  // Setup callbacks for SerialCommand commands
  sCmd.addCommand("High",    sendFullSpeed);
  sCmd.addCommand("Medium",     sendMediumSpeed);
  sCmd.addCommand("Low",   sendLowSpeed);
  sCmd.addCommand("Timer", sendTimer);
  sCmd.addCommand("Learn", sendRegister);        // Register remote in ithon fan
  sCmd.setDefaultHandler(unrecognized);      // Handler for command that isn't matched  (says "What?")
  server.on("/", handle_root);

  server.on("/api", handle_api);
  server.on("/button", handle_buttons);




  if (!mdns.begin(espName.c_str(), WiFi.localIP())) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }

  server.begin();
  Serial.println("HTTP server started");
  MDNS.addService("http", "tcp", 80);
  rf.initReceive(); //put in receive mode

  //start DHT sensor
  dht.begin();
  Serial.println("DHT sensor started");

  //MQTT
  mqttclient.setServer(mqttServer, 1883);
  mqttclient.setCallback(callback);
Serial.println("MQtt server");

}




// LOOP
void loop(void)
{
  sCmd.readSerial();
  server.handleClient();

  if (rf.checkForNewPacket())
  {

    my_interrupt_handler();

  }

  if (timerx10 >= 1 && (millis() - time2) == (timerx10 * 600000))  //every push of timer is 10 minutes
  {


    Serial.println("time");
    time2 = millis();           //and reset time.
    timerx10 = 0;
    Serial.println("Returning to low.");
    eepromWrite(0, 6, "Low");
    handle_root();

  }

  if (millis() - dht_lastInterval > dht_sendInterval && DHTEnabled == 1)
  {
    handle_DHT();
    dht_lastInterval = millis();
  }
  if (!mqttclient.connected()) {
    reconnect();
  }
  mqttclient.loop();

}


// VOID setups


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
 
  Serial.println("payload");
  payload[length] = '\0';
  String strPayload = String((char*)payload);
  Serial.println(strPayload);
  
    if (strPayload == "Learn") {
    sendRegister();
    }
    else if (strPayload == "High"){
    sendFullSpeed();
    }
        else if (strPayload == "Medium"){
    sendMediumSpeed();
    }
        else if (strPayload == "Low"){
    sendLowSpeed();
    }
        else if (strPayload == "Timer"){
    sendTimer();
    }
        else if (strPayload == "Reset"){
    ESP.restart();
    }
  }



void reconnect() {
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttclient.connect(mqttClientId, mqttUsername, mqttPassword)) {
      Serial.println("connected");
      // subscribe
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void my_interrupt_handler()
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200)
  {
    packet = rf.getLastPacket();
    //show counter
    Serial.print("counter=");
    Serial.print(packet.counter);
    Serial.print(", ");
    //show command
    switch (packet.command) {
      case IthoUnknown:
        Serial.print("unknown\n");
        break;
      case IthoLow:
        Serial.print("low\n");
        eepromWrite(0, 6, "Low");
        break;
      case IthoMedium:
        Serial.print("medium\n");
        eepromWrite(0, 6, "Medium");
        break;
      case IthoFull:
        Serial.print("full\n");
        eepromWrite(0, 6, "Full");
        break;
      case IthoTimer1:
        Serial.print("timer1\n");
        eepromWrite(0, 6, "Timer");
        time2 = millis();
        ++timerx10;
        break;
      case IthoTimer2:
        Serial.print("timer2\n");
        break;
      case IthoTimer3:
        Serial.print("timer3\n");
        break;
      case IthoJoin:
        Serial.print("join\n");
        break;
      case IthoLeave:
        Serial.print("leave\n");
        break;
    } // switch (recv) command
    // checkfornewpacket
    yield();
  }
  last_interrupt_time = interrupt_time;
}



void handle_api()
{
  // Get var for all commands
  String action = server.arg("action");
  String value = server.arg("value");
  String api = server.arg("api");

  if (action == "Receive")
  {
    rf.initReceive();
    server.send ( 200, "text/html", "Receive mode");
  }

  if (action == "High")
  {
    sendFullSpeed();
    time2 = millis();
    server.send ( 200, "text/html", "Full Powerrr!!!");
  }

  if (action == "Medium")
  {
    sendMediumSpeed();
    time2 = millis();
    server.send ( 200, "text/html", "Medium speed selected");
  }

  if (action == "Low")
  {
    sendLowSpeed();
    time2 = millis();
    server.send ( 200, "text/html", "Slow speed selected");
  }

  if (action == "Timer")
  {
    sendTimer();
    server.send ( 200, "text/html", "Timer on selected");
  }

  if (action == "Learn")
  {
    sendRegister();
    server.send ( 200, "text/html", "Send learn command OK");
  }


  if (action == "reset" && value == "true")
  {
    server.send ( 200, "text/html", "Reset ESP OK");
    delay(500);
    Serial.println("RESET");
    ESP.restart();
  }

  if (action == "DHT")
  {
    handle_DHT();
    server.send ( 200, "text/html", "DHT");
  }
}

void handle_buttons()
{
  // Get vars for all commands
  String action = server.arg("action");
  String api = server.arg("api");

  if (action == "High")
  {
    handle_root();
    sendFullSpeed();
  }

  if (action == "Medium")
  {
    handle_root();
    sendMediumSpeed();
  }

  if (action == "Low")
  {
    handle_root();
    sendLowSpeed();
  }

  if (action == "Timer")
  {
    handle_root();
    sendTimer();
  }

  if (action == "Learn")
  {
    handle_root();
    sendRegister();
  }

}


void sendRegister() {
  Serial.println("sending join...");
  rf.sendCommand(IthoJoin);
  rf.initReceive(); //turn back in receive mode
  Serial.println("sending join done.");
  //handle_root();
}

void sendLowSpeed() {
  Serial.println("sending low...");
  rf.sendCommand(IthoLow);
  rf.initReceive(); //turn back in receive mode
  Serial.println("sending low done.");
  eepromWrite(0, 6, "Low");
  time2 = millis();
  timerx10 = 0;
  //handle_root();
}

void sendMediumSpeed() {
  Serial.println("sending medium...");
  rf.sendCommand(IthoMedium);
  rf.initReceive(); //turn back in receive mode
  Serial.println("sending medium done.");
  eepromWrite(0, 6, "Medium");
  time2 = millis();
  timerx10 = 0;
  // handle_root();
}

void sendFullSpeed() {
  Serial.println("Full speed captain!");
  rf.sendCommand(IthoFull);
  rf.initReceive(); //turn back in receive mode
  Serial.println("Now running at maximum speed!");
  eepromWrite(0, 6, "High");

  time2 = millis();
  timerx10 = 0;
  //  handle_root();
}

void sendTimer() {
  Serial.println("sending timer...");
  rf.sendCommand(IthoTimer1);
  rf.initReceive(); //turn back in receive mode
  Serial.println("sending timer done.");
  eepromWrite(0, 6, "Timer");
  time2 = millis();
  ++timerx10;
  Serial.println("Timer state: : ");
  Serial.println(timerx10);
  // handle_root();
}

void handle_DHT() {
  float h = dht.readHumidity();
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(f)) {
    return;
  }
  //   mqttclient.publish("m_bed/temperature",String(f));
  //  mqttclient.publish("m_bed/humidity",String(h));



}


void unrecognized(const char *command) {
  Serial.println("What?");
}



