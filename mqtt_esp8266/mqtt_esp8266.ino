
/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

// Update these with values suitable for your network.

#define        LED_RED             D8
#define        LED_GREEN           D6
#define        LED_BLUE            D7
#define        LDR                 A0
#define        BUTTON              D2
#define        SERVO               D1

const char* ssid = "Free_Wifi";
const char* passphrase = "lamgicopass";
const char* mqtt_server = "m11.cloudmqtt.com";
const char* clientID = "ESP_Client";
const char* user = "uglywolf";
const char* password = "123456";

WiFiClientSecure espClient;
PubSubClient client(espClient);
Servo myServo;

long lastMsg = 0;
char msg[50];
unsigned long timer = 0;
int flag = 0;
int analogValue;
const char* fingerprint = "A5 02 FF 13 99 9F 8B 39 8E F1 83 4F 11 23 65 0B 32 36 FC 07";

void setup()
{
  //change WIFI mode into STATION
  WiFi.mode(WIFI_STA);

  //Init I/O pin
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(BUTTON, INPUT);

  //Turn on interrupt
  attachInterrupt(BUTTON, interruptHandler, FALLING);

  //Init Servo
  myServo.attach(SERVO);
  
  Serial.begin(115200);
  setupWifi();
  client.setServer(mqtt_server, 25843);
  client.setCallback(callback);
}

//Interrupt Handler
void interruptHandler()
{
  detachInterrupt(BUTTON);
  if ((millis() - timer) < 0)
    timer = 0;
  if ((millis() - timer) < 100)
    return;

  timer = millis();
  flag = 1;
}

//Setup Wifi network
void setupWifi() 
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, passphrase);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '0')
  {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_BLUE, LOW);
  } else
  {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, HIGH);
    
    myServo.write(180);
    delay(1000);
    myServo.write(0);

    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_BLUE, LOW);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(clientID, user, password, "Status", 0, true, "Disconnected"))
    {
      if (espClient.verify(fingerprint, mqtt_server))
      {
        Serial.println("connected");
        // Once connected, publish an announcement...
        client.publish("Status", "Connected", true);
        // ... and resubscribe
        client.subscribe("Control");
      }
      else
      {
        Serial.println("Failed to verify server");
        client.disconnect();
      }
    } else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  
  //Action when having an interrupt signal
  if (flag == 1)
  {
    analogValue = analogRead(LDR);
    int servoPos = map(analogValue, 0, 1023, 0, 180);
    myServo.write(180);
    delay(1000);
    myServo.write(0);
    
    client.publish("Sensor", String(analogValue).c_str());
    flag = 0;
    attachInterrupt(BUTTON, interruptHandler, FALLING);
  }
  
  client.loop();
}
