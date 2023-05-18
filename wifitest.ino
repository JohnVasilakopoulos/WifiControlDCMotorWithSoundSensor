/*
  WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.

  Steps:
  1. Connect to the access point "yourAp"
  2. Point your web browser to http://192.168.4.1/H to turn the LED on or http://192.168.4.1/L to turn it off
     OR
     Run raw TCP "GET /H" and "GET /L" on PuTTY terminal with 192.168.4.1 as IP address and 80 as port

  Created for arduino-esp32 on 04 July, 2018
  by Elochukwu Ifediora (fedy0)
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#define LED_BUILTIN 13   // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED
#define EncoderOutput 823

// Set these to your desired credentials.
const char *ssid = "yourAP";
const char *password = "yourPassword";


int motorSpeed = 33;  // PWM pin
int motorDir1  = 21;  // Pin 2 of L293
int motorDir2  = 32;  // Pin 7 of L293
int motorCtrl1 = 15;
int motorCtrl2 = 27;

volatile long int encoder_pos = 0;
volatile long encoderValue = 0;

int interval = 1000;
long PrevTime = 0;
long CurrentTime = 0;
int rpm = 0;

// PWM properties
const int freq = 30000;      
const int ledChannel = 0;
const int resolution = 8;


int       motorDirection = 1;//default direction of rotation
const int motorStep = 5;// 5 is 5/255 every time button is pushed
int       motorCurrentSpeed = 200;// variable holding the light output value (initial value) 200 means 200/255
const int minSpeed=20; //Minimum speed of the motor
const int maxSpeed=255; // Maximum speed of the motor
int       motorState=HIGH; //Displays state of motor (HIGH means STOP) and LOW means Start
int       motorSwitch = 1; // Switches the state of motor (1 means HIGH) and 0 means LOW
const int motorStop = 0;

WiFiServer server(80);


void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(motorDir1, OUTPUT);
  pinMode(motorDir2, OUTPUT);
  pinMode(motorSpeed, OUTPUT);

  pinMode(motorCtrl1, INPUT);  
  pinMode(motorCtrl2, INPUT);  
  attachInterrupt(digitalPinToInterrupt(motorCtrl1), encoder, RISING);
  
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(motorSpeed, ledChannel);

  digitalWrite(motorDir1,HIGH);
  digitalWrite(motorDir2,LOW);

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();

  Serial.println("Server started");
  PrevTime= millis();
  encoderValue = 0;
}

void loop() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> to turn ON the LED.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn OFF the LED.<br>");
            client.print("Click <a href=\"/F\">here</a> to make the motor go faster.<br>");
            client.print("Click <a href=\"/S\">here</a> to make the motor go slower.<br>");
            client.print("Click <a href=\"/D\">here</a> to make the motor switch directions.<br>");
            client.print("Click <a href=\"/O\">here</a> to turn ON/OFF the motor.<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_BUILTIN, LOW);                // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /F")) {            // GET /F speeds up the motor
          if (motorState == LOW ) {
            if (motorCurrentSpeed == maxSpeed) {
              motorCurrentSpeed=maxSpeed;
            }
            else {
              motorCurrentSpeed=motorCurrentSpeed+motorStep;
            }
            ledcWrite(ledChannel, motorCurrentSpeed);  
          }
        }
       if (currentLine.endsWith("GET /S")) {             // GET /S slows down the motor
          if (motorState == LOW ) {
            if (motorCurrentSpeed == minSpeed) {
              motorCurrentSpeed=minSpeed;
            }
            else {
              motorCurrentSpeed=motorCurrentSpeed-motorStep;
            }
            ledcWrite(ledChannel, motorCurrentSpeed);
          }
       }
       if (currentLine.endsWith("GET /D")) {            // GET /D changes the direction of the motor
        if (motorDirection == 1){
          digitalWrite(motorDir1,LOW);
          digitalWrite(motorDir2,HIGH);
          motorDirection = 0;
        }
        else{
          digitalWrite(motorDir1,HIGH);
          digitalWrite(motorDir2,LOW);
          motorDirection=1;
        }
       }
       if (currentLine.endsWith("GET /O")) {           // GET /O turns on/off the motor
        if (motorSwitch == 1){
          motorState = LOW;
          motorSwitch = 0;
          ledcWrite(ledChannel, motorCurrentSpeed);
          
        }
        else{
          motorState = HIGH;
          motorSwitch = 1;
          ledcWrite(ledChannel,motorStop);
        }
       }
      }
      //Serial.println(encoder_pos);

    }
    // close the connection:
    client.stop();
    
    Serial.println("Client Disconnected.");
  }
      CurrentTime = millis();
      if (CurrentTime - PrevTime > interval){
        PrevTime = CurrentTime;
        rpm = (float)(encoderValue * 60 / EncoderOutput);
        if (rpm > 0) {
          Serial.print(encoderValue);
          Serial.print(" pulse / ");
          Serial.print(EncoderOutput);
          Serial.print(" pulse per rotation x 60 seconds = ");
          Serial.print(rpm);
          Serial.println(" RPM");
        }
        encoderValue = 0;
      }
}

void encoder(){
  encoderValue++;
  if (digitalRead(motorCtrl2) == HIGH){
    encoder_pos++;
  }else{
    encoder_pos--;
  }
}
