#include "secrets.h"
#include<ESP8266WiFi.h>
#include<PubSubClient.h>

// Defines
#define SAMPLES 40
#define ACT_THRESHOLD 1.5
#define TIMEOUT 2000

// Ultrasonic sensor
const int trigPin = 4;
const int echoPin = 5;
const int ledPin = 2;
float duration, distance;

// Digital Signal Processing
float cal = 0;
float threshold = 25.0;
float data[SAMPLES];
unsigned long next_read_start = 0;
unsigned long next_status = 0;

// WiFi
const char ssid[] = SECRET_SSID; 
const char pass[] = SECRET_PASS;
WiFiClient espClient;

// MQTT
const char *mqtt_broker = "broker.emqx.io";
const char *mqtt_topic = "SIT210/wave";
const int mqtt_port = 1883;
PubSubClient mqtt_client(espClient);

// See "MQTT Implementation Help" in README
void connectToWiFi() {
    WiFi.begin(ssid, pass);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to the WiFi network");
}

// See "MQTT Implementation Help" in README
void connectToMQTTBroker() {
    while (!mqtt_client.connected()) {
        String client_id = "esp8266-client-" + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str())) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic);
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

// MQTT Callback
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  // Recover the message as a string
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char) payload[i];
  }

  // Act on the recovered string
  if(msg == "pat"){
    Serial.println("Pat!");
    flash(2, 3.5);
  }else if(msg == "wave"){
    Serial.println("Wave!");
    flash(2, 3.5);
  }
}

// Handle actions occuring
void handleAction(int status){
  switch(status){
    case 1: // Pat
      mqtt_client.publish(mqtt_topic, "pat");
      break;
    case 2: // Wave
      mqtt_client.publish(mqtt_topic, "wave");
      break;

    default: // If status is unknown, error -2
      handleError(-2);
      break;
  }
}

// Handle actions locally to provide immediate feedback during testing
void handleActionLocal(int status){
  switch(status){
    case 1: // Pat
      Serial.println("Pat!");
      flash(2, 3.5);
      break;
    case 2: // Wave
      Serial.println("Wave!");
      flash(3, 2.5);
      break;

    default:
      handleError(-2);
      break;
  }
}

// Flash a provided pattern profile
void flash(int flashes, float length){
  for(int i = 0; i < flashes; i++){
    digitalWrite(ledPin, HIGH);
    delay(length * 100);
    digitalWrite(ledPin, LOW);
    delay(length * 100);
  }
}

// Handle error codes
void handleError(int code){
  switch(code){
    case -1: // Timeout not reached, ignore this error
      break;
    case -2: // handleAction received an unknown status
      Serial.println("Unknown status given to action handler!");
      break;
    default: // Handle unknown error codes
      Serial.print("Unknown error code: ");
      Serial.println(code);
      break;
  }
}

// Signal processing
int processSignals(float samples[SAMPLES]){
  // Return an error if last status is still valid
  if(millis() < next_status){
    return -1;
  }

  // Variables for the signal processing
  int ret = 0;
  int trig_idx = -1;
  int end_idx = -1;
  int trig_samples = 0;
  float avg_btw = 0;
  float min = cal;

  // Iterate through the passed samples and perform some magic
  for(int i = 0; i < SAMPLES; i++){
    if(trig_idx < 0){
      if(cal - samples[i] > threshold){
        trig_idx = i;
      }
    }else if(trig_idx > -1){
      if(end_idx < 0){
        if(cal - samples[i] <= threshold){
          end_idx = i;
        }
      }else{
        trig_samples = end_idx - trig_idx;
        for(int j = trig_idx; j < end_idx; j++){
          avg_btw += samples[j];
          if(samples[j] < min){
            min = samples[j];
          }
        }
        avg_btw = avg_btw / trig_samples;
        break;
      }
    }
  }

  // Update the next_status
  next_status = millis() + TIMEOUT;
  if(avg_btw > 0){
    if(avg_btw - min > ACT_THRESHOLD){
      ret = 1;
    }else{
      if(trig_samples < 8){
        ret = 2;
      }
    }
  }

  // Reset the data array if a status has been set
  if(ret != 0){
    for(int i = 0; i < SAMPLES; i++){
      data[i] = cal;
    }
  }

  return ret;
}

// Calibration for the signal processing
float calibrate(){
  Serial.println("Calibrating, do not block sensor!");
  delay(2500);
  float avg = 0;
  int cal_points = 10;
  for(int i = 0; i < cal_points; i++){
    avg += read_dist();
  }
  avg = avg / cal_points;
  Serial.print("Calibration complete. Calibration value: ");
  Serial.print(avg);
  Serial.println("cm.");
  return avg;
}

// Read the distance in cm from the ultrasonic sensor
float read_dist(){
  while(millis() < next_read_start){} // Wait for the start of the next read window
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration*.0343)/2;
  next_read_start = millis() + 50; // Minimum delay for an accurate reading
  return distance;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Initialising...");
  digitalWrite(ledPin, HIGH); // LED signals something is happening

  // Configure pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);

  // Configure WiFi and MQTT
  connectToWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();

  // Calibrate sensor for signal processor
  cal = calibrate();

  // Prefill data array
  for(int i = 0; i < SAMPLES; i++){
    data[i] = cal;
  }

  digitalWrite(ledPin, LOW); // LED signals that setup is complete
}

void loop() {
  // First handle our loop
  for(int i = 1; i < SAMPLES; i++){
    data[i-1] = data[i];
  }

  float dist = read_dist();
  data[SAMPLES-1] = dist;

  int status = processSignals(data);

  if(status > 0){
    handleAction(status);
  }else if(status < 0){
    handleError(status);
  }

  // Now check MQTT
  if (!mqtt_client.connected()) {
    connectToMQTTBroker();
  }
  mqtt_client.loop();
}