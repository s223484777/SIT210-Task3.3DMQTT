# SIT210-Task5.1PGUI
 S306 SIT210 Task 3.3D, detect a wave and a pat using an ultrasonic sensor

## Description
A simple sensor with some (basic and probably janky) signal processing to detect and react to waves and pats.

A wave is considered as from zero to many swipes through the sensor range.
A pat is considered as the distance decreasing and then increasing multiple times.
A breakdown is considered as my existance

When an action is detected (either a wave or a pat), a message is sent to the MQTT broker.
When an MQTT message is recieved, the LED blinks, a message is sent to the serial console.
The LED blink pattern varies based on the message.

## How it Works
Due to my ultrasonic sensor being not the greatest quality, I have utilised an ESP8266 based D1 mini clone for this task. 

The first step is to find the calibration value by averaging a number of distance measurements. This calibration value is used by the signal processor to detect when something interupts the sensor field of view, and almost mark the distance to the wall that the sensor is pointed to so that it doesn't trigger any action.
In the main `loop`, the `data` array is first rotated left and then the latest distance reading added. This ensures that the signal processor can use the latest data for each analysis.

Signal processors work primarily on magic, but mine works mostly out of spite.
The main processing works by finding the start and end of a "drop" in distance, which indicates that something is within the sensor range.
The `trig_idx` value records the drop in distance, and the `end_idx` records the recovery after an action has taken place.
We then average the samples in the drop and record the minimum if we have both a start and end, and leave the loop.
```cpp
  for(int i = 0; i < SAMPLES; i++){
    if(trig_idx < 0){
      if(cal - samples[i] > threshold){
        trig_idx = i; // Drop found
      }
    }else if(trig_idx > -1){
      if(end_idx < 0){
        if(cal - samples[i] <= threshold){
          end_idx = i; // Recovery found
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
```

Now that we have the average and minimum values, we can determine if the difference between the average and the minimum exceeds the threshold.
A "pat" will see the average be higher than the minimum, whereas a "wave" will remain at a fairly constant distance and only have a small number of samples. 
```cpp
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
```

If we have found a valid action and are going to return a non-zero status, we also need to reset the data array to prevent another pass from detecting the same action just in a different location (since the array is rotated, a second action will be "earlier" in the data array.
```cpp
  if(ret != 0){
    for(int i = 0; i < SAMPLES; i++){
      data[i] = cal;
    }
  }
```

## Usage
Requires the following libraries:
* ESP8266WiFi
* PubSubClient

## MQTT Implementation Help
A fantastic reference which I used to help develop the MQTT component of this task: [Using MQTT on ESP8266: A Quick Start Guide | EMQ](https://www.emqx.com/en/blog/esp8266-connects-to-the-public-mqtt-broker)
The only changes made were to remove the username and password for the default public user.