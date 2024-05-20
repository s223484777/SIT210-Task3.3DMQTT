#define SAMPLES 40
#define ACT_THRESHOLD 1.5
#define TIMEOUT 2000

const int trigPin = 4;
const int echoPin = 5;
const int ledPin = 2;
float duration, distance;

float cal = 0;
float threshold = 25.0;
float data[SAMPLES];
unsigned long next_read_start = 0;
unsigned long next_status = 0;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
  Serial.println("Initialising...");
  digitalWrite(ledPin, HIGH);

  delay(2500);
  cal = calibrate();

  // Prefill data array
  for(int i = 0; i < SAMPLES; i++){
    data[i] = cal;
  }

  digitalWrite(ledPin, LOW);
}

void loop() {
  float dist = read_dist();
  for(int i = 1; i < SAMPLES; i++){
    data[i-1] = data[i];
  }
  data[SAMPLES-1] = dist;

  int status = processSignals(data);

  if(status > 0){
    handleAction(status);
  }else if(status < 0){
    handleError(status);
  }
}

void handleAction(int status){
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
      Serial.println("Unknown status, aborting!");
      break;
  }
}

void flash(int flashes, float length){
  for(int i = 0; i < flashes; i++){
    digitalWrite(ledPin, HIGH);
    delay(length * 100);
    digitalWrite(ledPin, LOW);
    delay(length * 100);
  }
}

void handleError(int code){
  switch(code){
    case -1:
      break; // timeout not reached, ignore this error
    default:
      Serial.print("Unknown error code: ");
      Serial.println(code);
  }
}

int processSignals(float samples[SAMPLES]){
  if(millis() < next_status){
    return -1;
  }

  int ret = 0;
  int trig_idx = -1;
  int end_idx = -1;
  int trig_samples = 0;
  float avg_btw = 0;
  float min = cal;

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

  if(ret != 0){
    for(int i = 0; i < SAMPLES; i++){
      data[i] = cal;
    }
  }

  return ret;
}

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