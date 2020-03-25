/***
 * TODO
 * - On/Off power switch
 * - Fine tuning up/down switch
 * - Pause switch (so we can take something off the scale without triggering next fill
 * 
 * x0: -70095, -70348
 * x1: -336112, -336373
 * 
  x0 = -70348;
  x1 = -336373;
 */


// Load Cell Library
#include <HX711.h>

/***** PROTOTYPE PINS
// Load cell pin definitions
#define LOADCELL_DOUT_PIN  3
#define LOADCELL_SCK_PIN  2

// Base pin definition for TIP120 transistor (controls pump on/off)
#define TRANS_BASE_PIN 11

// Rotation Speed Pin (controls pump rotation speed)
#define ROTATION_SPEED_PIN 5

// Pause Button Pin
#define PAUSE_SWITCH_PIN 8
*/

/***** PERMANENT PINS */
// Load cell pin definitions
#define LOADCELL_DOUT_PIN  12
#define LOADCELL_SCK_PIN  11

// Base pin definition for TIP120 transistor (controls pump on/off)
#define TRANS_BASE_PIN 3

// Rotation Speed Output Pin (controls pump rotation speed)
#define ROTATION_SPEED_PIN 5

// Rotation Speed Potentiometer Pin
#define ROTATION_SPEED_POT_PIN A5

// Pause Button Pin
#define PAUSE_SWITCH_PIN 2


// Variables used for load cell calibration
float y1 = 20.353;
long x0 = -70348;
long x1 = -336373;
float target_mass;
float current_mass;
int sample_size = 5;

float fill_diff = 0;

// Variables controlling pump rotation speed
int rotation_speed = 0;
int rotation_speed_pot_val = 0;
bool slow_zone = false;

// Variables controlling running/paused state
int paused = 0;

HX711 scale;

void setup() {

  // Activate Serial output
  Serial.begin(9600);
  
  pinMode(LED_BUILTIN, OUTPUT);       // LED pin
  pinMode(TRANS_BASE_PIN, OUTPUT);           // Transistor base pin
  pinMode(ROTATION_SPEED_PIN, OUTPUT);  // Rotation speed pin
  pinMode(ROTATION_SPEED_POT_PIN, INPUT);  // Rotation speed potentiometer pin
  pinMode(PAUSE_SWITCH_PIN, INPUT);     // Pause button pin

  // Initialize Scale
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // Read the target mass
  setTargetMass();
}

void loop()
{  
  current_mass = calculate_mass(scale.read());
  Serial.print("Current weight: ");
  Serial.println(current_mass);

  // Slow down the pump speed if we are approaching the target weight
  if (!slow_zone && current_mass > target_mass * 0.95) {
    Serial.println("Entering Slow Zone");
    slow_zone = true;
  }
  
  // Slow down the pump speed if we are approaching the target weight
  if (slow_zone) {
    analogWrite(ROTATION_SPEED_PIN, rotation_speed_pot_val * 0.33);
  } else {
    //rotation_speed_pot_val = analogRead(ROTATION_SPEED_POT_PIN);
    rotation_speed_pot_val = rpm_analog_val(600);
    analogWrite(ROTATION_SPEED_PIN, rotation_speed_pot_val);
  }

  if (current_mass >= target_mass - fill_diff) {
    togglePump(false);
    
    printCurrentWeight(current_mass);

    delay(1000);
    
    fill_diff += calculate_mass(getReading(0)) - target_mass;
    if (fill_diff > 0.1 || fill_diff < -0.1) {
      fill_diff = 0;
    }
    Serial.println(fill_diff);
    
    digitalWrite(LED_BUILTIN, HIGH);
    while (true) {
      printCurrentWeight(calculate_mass(getReading(0)));
      delay(100);
      
      current_mass = calculate_mass(getReading(0));
      if (current_mass < target_mass * 0.85) {
        digitalWrite(LED_BUILTIN, LOW);

        Serial.println();
        Serial.println("Starting pump...");
        printCoundown(3);
        
        current_mass = calculate_mass(getReading(0));
        
        printCurrentWeight(current_mass);
        printCurrentWeight(target_mass * 0.2);
        
        if (current_mass > target_mass * 0.2) {
          togglePump(true);
          return;
        }
      }
    }
  }
}

void togglePump(bool on)
{
  if (on) {
    Serial.println("Toggling Pump On");
    digitalWrite(TRANS_BASE_PIN, 255);
//    analogWrite(ROTATION_SPEED_PIN, 255);
    slow_zone = false;
  } else {
    Serial.println("Toggling Pump Off");
    digitalWrite(TRANS_BASE_PIN, 0);
  }
}

void printCurrentWeight(float current_weight)
{
  Serial.print("Current weight: ~");
  Serial.print(current_weight);
  Serial.print(" oz");
  
  Serial.print("; Target weight: ~");
  Serial.print(target_mass);
  Serial.print(" oz");
  
  Serial.println();
}

void calibrate()
{
  /*
  Serial.println("Remove all mass");
  printCoundown(5);
  Serial.println("Reading Average");
  x0 = getReading(10);
  blink(2);
  Serial.println(x0);
  
  Serial.println("Add Calibration Mass");
  printCoundown(5);
  Serial.println("Reading Average");
  x1 = getReading(10);
  blink(2);
  Serial.println(x1);
  */
  
  Serial.println("Add Target Mass");
  printCoundown(7);
  Serial.println("Reading Average");
  target_mass = calculate_mass(getReading(10));
  blink(2);
  Serial.println();

//  target_mass = 10.36;
  
  Serial.print("Calibration Complete. Target weight: ");
  Serial.println(target_mass);
}

void setTargetMass()
{
  Serial.println("Add Target Mass");
  printCoundown(7);
  Serial.println("Reading Average");
  target_mass = calculate_mass(getReading(10));
  blink(2);
  Serial.println();

  Serial.print("Calibration Complete. Target weight: ");
  Serial.println(target_mass);
  Serial.println();
}

float getReading(int read_delay)
{
  long reading = 0;
  for (int j = 0; j < sample_size; j++) {
    reading += scale.read();
    if (read_delay > 0) {
      delay(read_delay);
    }
  }
  
  return reading / long(sample_size);
}

float calculate_mass(long reading)
{
  // calculating mass based on calibration and linear fit
  float ratio_1 = (float) (reading - x0);
  float ratio_2 = (float) (x1 - x0);
  float ratio = ratio_1/ratio_2;
  float mass = y1 * ratio;

  return mass;
}

int rpm_analog_val(int rpm)
{
  return rpm * 0.425;
}

void checkPause()
{
  return;
  if (digitalRead(PAUSE_SWITCH_PIN) == HIGH) {
    Serial.println("Paused");
    while(true) {
      if (digitalRead(PAUSE_SWITCH_PIN) == LOW) {
        Serial.println("Unpaused");
        break;
      }
    }
  }
}

void printCoundown(int seconds)
{
  for (int i = seconds; i > 0; i--) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    if (i > 1) {
      digitalWrite(LED_BUILTIN, LOW);
    }
    Serial.print(i);
    Serial.print("..");
    delay(950);
  }
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.println();
  Serial.println();
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
