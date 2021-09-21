// Load Cell Library
#include <HX711.h>

// Load cell pin definitions
#define LOADCELL_DAT_PIN  A5 // 12
#define LOADCELL_CLK_PIN  A4

// Base pin definition for the transistor (controls pump on/off)
#define TRANS_BASE_PIN 3

// Rotation Speed Output Pin (controls pump rotation speed)
#define ROTATION_SPEED_PIN 5

#define SENSOR_ECHO_PIN A2 // 9 // Pin to receive echo pulse
#define SENSOR_TRIG_PIN A3 // 8 // Pin to send trigger pulse

#define TARGET_UP_BUTTON_PIN 6
#define TARGET_DOWN_BUTTON_PIN 7
#define TARGET_LED_PIN 8

String command;

// Variables used for load cell calibration
float calibration_factor = 436.8;
float target_weight;
float current_weight;
int sample_size = 5;

float fill_diff = 0;

bool slow_zone = false;
float slow_zone_target;

// Variables controlling pump rotation speed
int top_rotation_speed = 600;
float restart_pause_time = 1000;
float slow_zone_threshold = 0.95;
float slow_zone_speed_percentage = 0.3;
int delay_before_diff_calc = 250;

int target_up_button_val;
int target_down_button_val;

HX711 scale;

void setup() {

  // Activate Serial output
  Serial.setTimeout(1);
  Serial.begin(115200);
  Serial.flush();

  // Define GPIO pin modes
  pinMode(LED_BUILTIN, OUTPUT); // LED pin
  pinMode(TRANS_BASE_PIN, OUTPUT); // Transistor base pin
  pinMode(ROTATION_SPEED_PIN, OUTPUT); // Rotation speed pin
  pinMode(SENSOR_ECHO_PIN, INPUT);
  pinMode(SENSOR_TRIG_PIN, OUTPUT);
  pinMode(TARGET_UP_BUTTON_PIN, INPUT);
  pinMode(TARGET_DOWN_BUTTON_PIN, INPUT);
  pinMode(TARGET_LED_PIN, OUTPUT);

  // Initialize & Calibrate Scale
  scale.begin(LOADCELL_DAT_PIN, LOADCELL_CLK_PIN);
  scale.set_scale(436.8);
  scale.tare();
  
  // Read the target weight
  setTargetWeight();

  slow_zone_target = target_weight - (abs(target_weight) * (1 - slow_zone_threshold));
}

void loop()
{
//  if (Serial.available() > 0) {
//    command = Serial.readStringUntil("\n");
//    Serial.print("Received. ");
//    
//    target_weight += 1;
//  
//    Serial.print("Target Weight: ");
//    Serial.println(target_weight);
//  }
  
//  return;
  
  current_weight = scale.get_units(10);
  printCurrentWeight(current_weight);
  
  // Slow down the pump speed if we are approaching the target weight
  int rotation_speed;
  if (current_weight > slow_zone_target) {
    rotation_speed = rpm_analog_val(top_rotation_speed) * slow_zone_speed_percentage;
    Serial.print("Slow Zone ... ");
    Serial.println(rotation_speed);
  } else {
    rotation_speed = rpm_analog_val(top_rotation_speed);
    Serial.print("Normal Speed ... ");
    Serial.println(rotation_speed);
  }
  analogWrite(ROTATION_SPEED_PIN, rotation_speed);

  if (!bottlePresent() || current_weight >= (target_weight - fill_diff)) {
    togglePump(false);

    delay(delay_before_diff_calc);
    
    fill_diff += scale.get_units(10) - target_weight;
    if (fill_diff > 0.1 || fill_diff < -0.1) {
      fill_diff = 0;
    }
    Serial.println(fill_diff);
    
    digitalWrite(LED_BUILTIN, HIGH);
    while (true) {
      
      target_up_button_val = digitalRead(TARGET_UP_BUTTON_PIN);  // read input value
      if (target_up_button_val == HIGH) {
        digitalWrite(TARGET_LED_PIN, HIGH);
        Serial.println("Increase target");
        target_weight += (0.1 / 0.03527396);
        do {
          target_up_button_val = digitalRead(TARGET_UP_BUTTON_PIN);  // read input value
        } while (target_up_button_val == HIGH);
      }
      
      target_down_button_val = digitalRead(TARGET_DOWN_BUTTON_PIN);  // read input value
      if (target_down_button_val == HIGH) {
        digitalWrite(TARGET_LED_PIN, HIGH);
        Serial.println("Decrease target");
        target_weight -= (0.1 / 0.03527396);
        do {
          target_down_button_val = digitalRead(TARGET_DOWN_BUTTON_PIN);  // read input value
        } while (target_down_button_val == HIGH);
      }
      
      digitalWrite(TARGET_LED_PIN, LOW);
      
      current_weight = scale.get_units(10);
      printCurrentWeight(current_weight);
      delay(100);
      
      if (bottlePresent() && current_weight < (target_weight - (abs(target_weight) * 0.01))) {
        digitalWrite(LED_BUILTIN, LOW);

        Serial.println();
        Serial.println("Starting pump...");
        delay(restart_pause_time);
        togglePump(true);
        return;

        // One last check to make sure a container is on the scale before filling is started
//        current_mass = calculate_mass(getReading(0));
//        if (current_mass > target_mass - (abs(target_mass) * 0.8)) {
//          togglePump(true);
//          return;
//        }
      }
    }
  }
}

void togglePump(bool on)
{
  if (on) {
    Serial.println("Toggling Pump On");
    digitalWrite(TRANS_BASE_PIN, HIGH);
    slow_zone = false;
  } else {
    Serial.println("Toggling Pump Off");
    digitalWrite(TRANS_BASE_PIN, LOW);
  }
}

void printCurrentWeight(float current_weight)
{
  Serial.print("Current weight: ~");
  Serial.print(current_weight * 0.03527396);
  Serial.print(" oz");
  
  Serial.print("; Target weight: ~");
  Serial.print(target_weight * 0.03527396);
  Serial.print(" oz");
  
  Serial.println();
}

void setTargetWeight()
{
  do {
    current_weight = scale.get_units(10);
    blink(1);
  } while (abs(current_weight) < 5);

  delay(500);
  
  target_weight = scale.get_units(10);
}

int getSensorDistance()
{
  delay(100); // Wait 50mS before next ranging
  digitalWrite(SENSOR_TRIG_PIN, LOW); // Set the trigger pin to low for 2uS
  delayMicroseconds(2);
  digitalWrite(SENSOR_TRIG_PIN, HIGH); // Send a 10uS high to trigger ranging
  delayMicroseconds(10);
  digitalWrite(SENSOR_TRIG_PIN, LOW); // Send pin low again
  int distance = pulseIn(SENSOR_ECHO_PIN, HIGH); // Read in times pulse
  distance /= 58;

  return distance;
}

bool bottlePresent()
{
  int distance = getSensorDistance();
  
  if (distance > 0 && distance < 10 ) {
    return true;
  }
  
  return false;
}

int rpm_analog_val(int rpm)
{
  return rpm * 0.425;
}

void blink(int blinks)
{
  for (int i = 0; i < blinks; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
}
