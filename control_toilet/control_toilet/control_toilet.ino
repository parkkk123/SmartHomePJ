/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp8266-relay-module-ac-web-server/

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#include "ESP8266WiFi.h"
#include "ESPAsyncWebServer.h"
#include <Time.h>

// Set to true to define Relay as Normally Open (NO)
#define RELAY_NO    true

// Set number of relays
#define NUM_RELAYS  1

// Assign each GPIO to a relay
int relayGPIOs[NUM_RELAYS] = {5};

// Replace with your network credentials
const char* ssid = ".I'm my father_2.4G";
const char* password = "pwcpp1234";


//Static IP address configuration
IPAddress staticIP(192, 168, 0, 51); //ESP static ip
IPAddress gateway(192, 168, 0, 1);   //IP Address of your WiFi Router (Gateway)
IPAddress subnet(255, 255, 255, 0);  //Subnet mask
IPAddress dns(192, 168, 0, 1);  //DNS
const char* deviceName = "Lighting #2";

const char* PARAM_INPUT_1 = "relay";
const char* PARAM_INPUT_2 = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);



/////////////////////////////////////////////////////////////////////////// Control Ultrasonic //////////////////////////////////////////////////////
const int pingPin_1 = D5;
const int inPin_1 = D6;

long max_Ultrasonic1;
/////////////////////////////////////////////////////////////////////////// End Control Ultrasonic //////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////// Get time  //////////////////////////////////////////////////////
int timezone = 7 * 3600; //ตั้งค่า TimeZone ตามเวลาประเทศไทย

int dst = 0; //กำหนดค่า Date Swing Time

/////////////////////////////////////////////////////////////////////////// End Get time  //////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////// Motion Sensor  //////////////////////////////////////////////////////
const int digitalPinMotionSensor = D8;
/////////////////////////////////////////////////////////////////////////// End Motion Sensor  //////////////////////////////////////////////////////


String messagePrint = "";

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
  %MAXVALUESENSOR%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?relay="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?relay="+element.id+"&state=0", true); }
  xhr.send();
}</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    for(int i=1; i<=NUM_RELAYS; i++){
      String relayStateValue = relayState(i);
      buttons+= "<h4>Relay #" + String(i) + " - GPIO " + relayGPIOs[i-1] + "</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(i) + "\" "+ relayStateValue +"><span class=\"slider\"></span></label>";
    }
    return buttons;
  }else if(var == "MAXVALUESENSOR"){
    time_t now = time(nullptr);
    struct tm* p_tm = localtime(&now);
  
    long duration, ultrasonic_distance;
    pinMode(pingPin_1, OUTPUT);
    digitalWrite(pingPin_1, LOW);
    delayMicroseconds(2);
    digitalWrite(pingPin_1, HIGH);
    delayMicroseconds(5);
    digitalWrite(pingPin_1, LOW);
    pinMode(inPin_1, INPUT);
    duration = pulseIn(inPin_1, HIGH);
    
    ultrasonic_distance = microsecondsToCentimeters(duration);
    
    return "<br><br> Max distance sensor : "+ String(max_Ultrasonic1) +" cm. , Time system is " + String(p_tm->tm_hour) +"<br> Lastest distance : " + String(ultrasonic_distance) + ", Status Motion : " + String(digitalRead(digitalPinMotionSensor)) + "<br> Message : " + messagePrint;
  }
  return String();
}

String relayState(int numRelay){
  if(RELAY_NO){
    if(digitalRead(relayGPIOs[numRelay-1])){
      return "";
    }
    else {
      return "checked";
    }
  }
  else {
    if(digitalRead(relayGPIOs[numRelay-1])){
      return "checked";
    }
    else {
      return "";
    }
  }
  return "";
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(9600);
  // Set all relays to off when the program starts - if set to Normally Open (NO), the relay is off when you set the relay to HIGH
  for(int i=1; i<=NUM_RELAYS; i++){
    pinMode(relayGPIOs[i-1], OUTPUT);
    if(RELAY_NO){
      digitalWrite(relayGPIOs[i-1], HIGH);
    }
    else{
      digitalWrite(relayGPIOs[i-1], LOW);
    }
  }

  // Connect to Wi-Fi
  WiFi.hostname(deviceName);
  WiFi.config(staticIP,dns, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/updateDistance", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("val")) {
        long newVal = request->getParam("val")->value().toInt();
        max_Ultrasonic1 = newVal;
    }
    request->send(200, "text/plain", "OK");
  });

  // Send a GET request to <ESP_IP>/update?relay=<inputMessage>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    String inputMessage2;
    String inputParam2;
    // GET input1 value on <ESP_IP>/update?relay=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1) & request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      inputParam2 = PARAM_INPUT_2;
      if(RELAY_NO){
        Serial.print("NO ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], !inputMessage2.toInt());
      }
      else{
        Serial.print("NC ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], inputMessage2.toInt());
      }
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage + inputMessage2);
    request->send(200, "text/plain", "OK");
  });

  //Send status 
  // Send a GET request to <ESP_IP>/status?relay=<inputMessage>
  server.on("/status", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/status?relay=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      int readStatus = !digitalRead(relayGPIOs[inputMessage.toInt()-1]);
      Serial.print("Read status : " + String(readStatus));
      request->send(200, "text/plain", String(readStatus));
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "Error");
  });
  // Start server
  server.begin();
//////////////////////////////////////////////////////////////////// Get First time sensor value  ////////////////////////////////////////////////////////////////////
  long duration, cm_1;
  pinMode(pingPin_1, OUTPUT);
  digitalWrite(pingPin_1, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin_1, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin_1, LOW);
  pinMode(inPin_1, INPUT);
  duration = pulseIn(inPin_1, HIGH);
  
  cm_1 = microsecondsToCentimeters(duration);
  

  
  max_Ultrasonic1 = cm_1;
  Serial.println("max #1 : " + String(max_Ultrasonic1));


  /////////////////////////////////////////// Get time //////////////////////////////////////
  configTime(timezone, dst, "pool.ntp.org", "time.nist.gov"); //ดึงเวลาจาก Server
  Serial.println("\nLoading time");
  while (!time(nullptr)) {
     Serial.print("*");
    delay(1000);
  }

  Serial.println("");
}

int stateMachine = -1;
int readMotion = 0;
long duration;
long cm_1;
void loop() {
 
  delay(100);
  pinMode(pingPin_1, OUTPUT);
  digitalWrite(pingPin_1, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin_1, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin_1, LOW);
  pinMode(inPin_1, INPUT);
  duration = pulseIn(inPin_1, HIGH);
  
  cm_1 = microsecondsToCentimeters(duration);
  
  Serial.println("cm_1 : " + String(cm_1));
  messagePrint = "cm_1 : " + String(cm_1);
  
  

  //แสดงเวลาปัจจุบัน

   time_t now = time(nullptr);

   struct tm* p_tm = localtime(&now);

   Serial.println("Show time now : " + String(p_tm->tm_hour) + " hr");
   //&& (p_tm->tm_hour >= 18 || p_tm->tm_hour <= 6)
  if(stateMachine==-1  ){
    if(cm_1 < (max_Ultrasonic1*0.8) && (p_tm->tm_hour >= 18 || p_tm->tm_hour <= 6)){
      stateMachine = 0;
      Serial.println("Found one pass #-1");
      messagePrint = "Found one pass #-1";
    }else if(digitalRead(digitalPinMotionSensor) && (p_tm->tm_hour >= 18 || p_tm->tm_hour <= 6)){
      stateMachine = 0;
      Serial.println("Found one pass #-1 with special condition");
      messagePrint = "Found one pass #-1 with special condition";
    }
  }

  if(stateMachine==0){
    digitalWrite(relayGPIOs[0], 0);
    stateMachine = 1;
    Serial.println("Light on #0");
    messagePrint = "Light on #0";
    delay(15000);
  }
  
  if(stateMachine==1){
    stateMachine = 2;
    Serial.println("Check one #1");
    //27s delay of motion sensor 
    readMotion = digitalRead(digitalPinMotionSensor);
    Serial.println("Status Motion (0: No Move,1: Moved) : " + String(readMotion));
    messagePrint = "Status Motion (0: No Move,1: Moved) : " + String(readMotion);
    if(readMotion){
      stateMachine = 1;
    }
    delay(1000);
  }

  if(stateMachine==2){
    Serial.println("Reset #2");
    messagePrint = "Reset #2";
    digitalWrite(relayGPIOs[0], 1);
    stateMachine = -1;
  }
  
  
}
