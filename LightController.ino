/* LightController.pde -- Arduino program to control lightings */

#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"

// no-cost stream operator as described at 
// http://sundial.org/arduino/?page_id=119
template<class T>
inline Print &operator <<(Print &obj, T arg)
{ obj.print(arg); return obj; }

// CHANGE THIS TO YOUR OWN UNIQUE VALUE
static uint8_t mac[] = { 0xDE, 0xAD, 0xCD, 0xEF, 0xFE, 0xEE };

// CHANGE THIS TO MATCH YOUR HOST NETWORK
static uint8_t ip[] = { 192, 168, 1, 210 };

// Number of loop function calls since program start modulo 100
static int t = 0;

#define LIGHT_COUNT 3

#define PREFIX ""

static int lightStatuses[LIGHT_COUNT];

static int outputs[LIGHT_COUNT] = {3, 5, 6};

WebServer webserver(PREFIX, 80);

// commands are functions that get called by the webserver framework
// they can read any posted data from client, and they output to server

void displayLightStatuses(WebServer &server, WebServer::ConnectionType type, bool addControls = false)
{
  Serial.println("outputPins");
  P(htmlHead) =
    "<html>"
    "<head>"
    "<title>Light Controller</title>"
    "<style type=\"text/css\">"
    "BODY { font-family: sans-serif }"
    "H1 { font-size: 14pt; text-decoration: underline }"
    "P  { font-size: 10pt; }"
    "</style>"
    "</head>"
    "<body>";

  int i;
  server.httpSuccess();
  server.printP(htmlHead);

  if (addControls)
    server << "<form action='" PREFIX "/form' method='post'>";
    
  // Prepare code for forms
  P(textInput1) = "<input type='text' name='";
  P(textInput2) = "' value ='";
  P(textInput3) = "'>";
    
  

  server << "<h1>Light Statuses</h1><p>";

  for (i = 0; i < LIGHT_COUNT; ++i)
  {
    server << "Light " << i << " : ";
    char lightStatus[4];
    itoa(lightStatuses[i], lightStatus, 10);
    if (addControls)
    {
      char pinName[4];
      pinName[0] = 'l';
      itoa(i, pinName + 1, 10);
      server.printP(textInput1);
      server.print(pinName);
      server.printP(textInput2);
      server.print(lightStatus);
      server.printP(textInput3);
    }
    else
      server << lightStatus;

    server << "<br/>";
  }

  server << "</p>";

  if (addControls)
    server << "<input type='submit' value='Submit'/></form>";

  server << "</body></html>";
}

void formCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  Serial.println("formCmd");
  if (type == WebServer::POST)
  {
    bool repeat;
    char name[16], value[16];
    do
    {
      Serial.println("Read POST param");
      repeat = server.readPOSTparam(name, 16, value, 16);
      Serial.println(String(name));
      if (name[0] == 'l')
      {
        int light = strtoul(name + 1, NULL, 10);
        int val = strtoul(value, NULL, 10);
        if (val <= 100 && val >= 0)
          lightStatuses[light] = val;
      }
    } while (repeat);

    server.httpSeeOther(PREFIX "/");
  }
  else
    displayLightStatuses(server, type, true);
}

void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  displayLightStatuses(server, type, false);  
}

void setup()
{
  // Serial
  Serial.begin(9600);
  
  // Set PWM frequency
  setPwmFrequency(6, 1);
  
  // default light values
  for (int i = 0; i < LIGHT_COUNT; ++i) {
    lightStatuses[i] = 0;
  }
  
  // Pin modes
  for (int i = 0; i < LIGHT_COUNT; i++) {
    pinMode(outputs[i], OUTPUT);
  }

  // Web server
  Ethernet.begin(mac, ip);
  webserver.begin();
  webserver.setDefaultCommand(&defaultCmd);
  webserver.addCommand("form", &formCmd);
}

void loop()
{
  // process incoming connections one at a time forever
  webserver.processConnection();
  
  // Set light statuses
  for (int i = 0; i < LIGHT_COUNT; ++i) {
    analogWrite(outputs[i], lightStatuses[i] * 2.55);
  }
//  for (int i = 0; i < LIGHT_COUNT; i++) {
//    if (t < lightStatuses[i]) {
//      digitalWrite(outputs[i], HIGH);
//    } else {
//      digitalWrite(outputs[i], LOW);
//    }
//  }
//  
//  // Increase t
//  if ( ++t > 99 )
//    t = 0;
}
/**
 * Divides a given PWM pin frequency by a divisor.
 * 
 * The resulting frequency is equal to the base frequency divided by
 * the given divisor:
 *   - Base frequencies:
 *      o The base frequency for pins 3, 9, 10, and 11 is 31250 Hz.
 *      o The base frequency for pins 5 and 6 is 62500 Hz.
 *   - Divisors:
 *      o The divisors available on pins 5, 6, 9 and 10 are: 1, 8, 64,
 *        256, and 1024.
 *      o The divisors available on pins 3 and 11 are: 1, 8, 32, 64,
 *        128, 256, and 1024.
 * 
 * PWM frequencies are tied together in pairs of pins. If one in a
 * pair is changed, the other is also changed to match:
 *   - Pins 5 and 6 are paired on timer0
 *   - Pins 9 and 10 are paired on timer1
 *   - Pins 3 and 11 are paired on timer2
 * 
 * Note that this function will have side effects on anything else
 * that uses timers:
 *   - Changes on pins 3, 5, 6, or 11 may cause the delay() and
 *     millis() functions to stop working. Other timing-related
 *     functions may also be affected.
 *   - Changes on pins 9 or 10 will cause the Servo library to function
 *     incorrectly.
 * 
 * Thanks to macegr of the Arduino forums for his documentation of the
 * PWM frequency divisors. His post can be viewed at:
 *   http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1235060559/0#4
 */
void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}
