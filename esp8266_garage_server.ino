#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// Pin used to hookup the relay and sensors.
#define RELAY_PIN 14
#define OPEN_SENSOR 12
#define CLOSED_SENSOR 13

#define RELAY_TIME 1000

// Place network credentials here.
#define SSID "Will_2.4G"
#define PASSWORD "horatio0522"

// CLOSED and CLOSING is already defined in ESP8266WiFi.h.
enum DoorState { OPEN, CLOSED1, OPENING, CLOSING1, STOPPED } state;

ESP8266WebServer server(80);

void setup(void)
{
  Serial.begin(115200);
  WiFi.begin(SSID, PASSWORD);

  // Waiting for connection.
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  Serial.print("\nConnected to ");
  Serial.println(SSID);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(OPEN_SENSOR, INPUT);
  pinMode(CLOSED_SENSOR, INPUT);
  digitalWrite(RELAY_PIN, LOW);

  server.on("/isopen", isOpen);
  server.on("/isclosed", isClosed);
  server.on("/trigger", trigger);

  // This is used so we can shorten '192.168.x.x' to 'garage.local'.
  if (MDNS.begin("garage"))
    Serial.println("MDNS responder started");

  server.onNotFound(notFound);
  server.begin();

  state = getState();
  Serial.printf("Door state initialized to %s.\n", stateToString());
  Serial.println("HTTP server started");
}

// Returns true if the door is open.
void isOpen()
{
  state = getState();
  
  if (state == OPEN)
    server.send(200, "application/json", "{\"state\":\"1\"}");
  else
    server.send(200, "application/json", "{\"state\":\"0\"}");

  Serial.printf("Current state: %s.\n", stateToString());
}

// Returns true if the door is closed.
void isClosed()
{
  state = getState();
  
  if (state == CLOSED1)
    server.send(200, "application/json", "{\"state\":\"1\"}");
  else
    server.send(200, "application/json", "{\"state\":\"0\"}");

  Serial.printf("Current state: %s.\n", stateToString());
}

// Trigger the garage door to activate.
void trigger()
{
  state = getState();
  
  if (state == OPEN)  
    state = CLOSED1;
  else if (state == CLOSED1)
    state = OPENING;
  else if (state != STOPPED)
    state = STOPPED;

  Serial.printf("Triggered door. State set to %s.\n", stateToString());
  server.send(200, "text/plain", "");
  
  digitalWrite(RELAY_PIN, HIGH);
  delay(RELAY_TIME);
  digitalWrite(RELAY_PIN, LOW);
}

// Reads data from sensors to determine the doors state.
DoorState getState()
{
  int isOpen = digitalRead(OPEN_SENSOR);
  int isClosed = digitalRead(CLOSED_SENSOR);

  Serial.printf("isOpen: %d\nisClosed: %d\n", isOpen, isClosed);

  if (!isOpen && !isClosed)
    return state; // If door is neither open or closed, return the current state (CLOSING1, OPENING, or STOPPED)
  return (DoorState)!isOpen;
}

char* stateToString()
{
  switch (state)
  {
  case OPEN:
    return "OPEN";
  case CLOSED1:
    return "CLOSED";
  case STOPPED:
    return "STOPPED";
  case CLOSING1:
    return "CLOSING";
  case OPENING:
    return "OPENING";
  default:
    return "UNKNOWN";
  }
}

void loop(void)
{
  server.handleClient();
}

void notFound()
{
  server.send(404, "application/json", "{\"message\":\"Page not found\"}");
}
