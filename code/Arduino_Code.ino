/**
 * @file Arduino_Code.ino
 * @brief Autonomous Fire Fighting Robot Control Firmware
 * @details This sketch implements the autonomous navigation, fire detection,
 *          obstacle avoidance, and fire extinguishing sequence for the robot.
 *          It interfaces with 3 IR Flame Sensors, an L293D Motor Driver, an
 *          HC-SR04 Ultrasonic Sensor, an SG90 Servo Motor, and a 5V/12V DC Water Pump.
 * 
 * @author Midhunesh R, Gopika J, Madhu Priya P, Methun Vijay S
 * @institution Dr. N.G.P. Institute of Technology, Coimbatore
 * @date June 2024
 */

#include <Servo.h>

// ==========================================
// PIN DEFINITIONS
// ==========================================

// IR Flame Sensors (Active LOW - DO pins of modules)
#define SENSOR_LEFT   A0
#define SENSOR_CENTER A1
#define SENSOR_RIGHT  A2

// L293D Motor Driver Control Pins
#define MOTOR_LEFT_Fwd   2  // Left Motor Forward
#define MOTOR_LEFT_Rev   3  // Left Motor Backward
#define MOTOR_RIGHT_Fwd  4  // Right Motor Forward
#define MOTOR_RIGHT_Rev  5  // Right Motor Backward

// SG90 Servo Motor Control Pin
#define SERVO_PIN        9  // PWM Pin for Sweep Servo

// DC Water Pump Control Pin (via Relay Module or Transistor Switch)
#define WATER_PUMP_PIN   7

// HC-SR04 Ultrasonic Sensor Pins (Obstacle Avoidance)
#define US_TRIG_PIN      11
#define US_ECHO_PIN      12

// ==========================================
// CONFIGURATION PARAMETERS
// ==========================================
const int CRITICAL_DISTANCE = 15; // Stop distance in cm (from obstacles or fire source)
const int SCAN_SPEED        = 150; // PWM or delay speed for rotation scan
const int SERVO_CENTER_POS  = 90;  // Default center position of servo

// ==========================================
// GLOBAL OBJECTS & VARIABLES
// ==========================================
Servo nozzleServo;
int distance = 0; // Measured obstacle/target distance in cm

// ==========================================
// FUNCTION DECLARATIONS
// ==========================================
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void stopRobot();
void extinguishFire();
int getDistance();

// ==========================================
// SETUP FUNCTION
// ==========================================
void setup() {
  // Initialize Serial Communication for Debugging
  Serial.begin(9600);
  Serial.println(F("==========================================="));
  Serial.println(F("   Autonomous Fire Fighting Robot Initializing   "));
  Serial.println(F("==========================================="));

  // Configure Sensor Pins as Input
  pinMode(SENSOR_LEFT, INPUT);
  pinMode(SENSOR_CENTER, INPUT);
  pinMode(SENSOR_RIGHT, INPUT);

  // Configure Motor Driver Pins as Output
  pinMode(MOTOR_LEFT_Fwd, OUTPUT);
  pinMode(MOTOR_LEFT_Rev, OUTPUT);
  pinMode(MOTOR_RIGHT_Fwd, OUTPUT);
  pinMode(MOTOR_RIGHT_Rev, OUTPUT);

  // Configure Ultrasonic Sensor Pins
  pinMode(US_TRIG_PIN, OUTPUT);
  pinMode(US_ECHO_PIN, INPUT);

  // Configure Water Pump Pin as Output
  pinMode(WATER_PUMP_PIN, OUTPUT);
  digitalWrite(WATER_PUMP_PIN, LOW); // Keep pump OFF initially

  // Attach and Initialize SG90 Servo Motor
  nozzleServo.attach(SERVO_PIN);
  nozzleServo.write(SERVO_CENTER_POS); // Set servo pointing straight forward

  delay(2000); // Guard time to allow sensors to stabilize
  Serial.println(F("System Ready. Commencing autonomous scan."));
}

// ==========================================
// MAIN LOOP FUNCTION (State Machine)
// ==========================================
void loop() {
  // 1. Read Distance from Ultrasonic Sensor
  distance = getDistance();
  
  // 2. Read IR Flame Sensors (Active LOW modules)
  bool fireLeft   = (digitalRead(SENSOR_LEFT) == LOW);
  bool fireCenter = (digitalRead(SENSOR_CENTER) == LOW);
  bool fireRight  = (digitalRead(SENSOR_RIGHT) == LOW);

  // Print debug log to Serial Monitor
  Serial.print(F("Dist: ")); Serial.print(distance); Serial.print(F("cm | "));
  Serial.print(F("L-Flame: ")); Serial.print(fireLeft); Serial.print(F(" | "));
  Serial.print(F("C-Flame: ")); Serial.print(fireCenter); Serial.print(F(" | "));
  Serial.print(F("R-Flame: ")); Serial.println(fireRight);

  // 3. Obstacle Handling (Safety Override)
  if (distance > 0 && distance < CRITICAL_DISTANCE && !fireCenter && !fireLeft && !fireRight) {
    Serial.println(F("[WARNING] Obstacle detected! Maneuvering to avoid..."));
    stopRobot();
    delay(500);
    moveBackward();
    delay(800);
    turnRight(); // Turn right to find open space
    delay(600);
    stopRobot();
    return; // Resume loop
  }

  // 4. Decision Matrix for Fire Navigation and Extinguishing
  if (fireCenter && fireLeft && fireRight) {
    // Fire is immediately in front of the robot (all sensors triggered)
    Serial.println(F("[ALERT] Large Fire detected ahead! Stopping and commencing extinguishing..."));
    stopRobot();
    extinguishFire();
  }
  else if (fireCenter) {
    // Fire detected in front
    if (distance > 0 && distance <= CRITICAL_DISTANCE) {
      Serial.println(F("[TARGET] In range of Fire. Stopping and commencing extinguishing..."));
      stopRobot();
      extinguishFire();
    } else {
      Serial.println(F("[NAV] Fire detected center. Moving Forward."));
      moveForward();
    }
  }
  else if (fireLeft) {
    // Fire detected to the left
    Serial.println(F("[NAV] Fire detected left. Steering Left."));
    turnLeft();
    delay(200); // Small pulse turn for calibration
    stopRobot();
  }
  else if (fireRight) {
    // Fire detected to the right
    Serial.println(F("[NAV] Fire detected right. Steering Right."));
    turnRight();
    delay(200); // Small pulse turn for calibration
    stopRobot();
  }
  else {
    // No fire detected - Scanning routine (Slow 360 rotation to locate fire source)
    Serial.println(F("[SCAN] Searching for fire sources..."));
    // Slow rotational scan
    turnRight();
    delay(150);
    stopRobot();
    delay(800); // Allow sensor reading stabilization
  }
  
  delay(100); // Control loop delay
}

// ==========================================
// NAVIGATION CONTROLS (L293D Interface)
// ==========================================

void moveForward() {
  digitalWrite(MOTOR_LEFT_Fwd, HIGH);
  digitalWrite(MOTOR_LEFT_Rev, LOW);
  digitalWrite(MOTOR_RIGHT_Fwd, HIGH);
  digitalWrite(MOTOR_RIGHT_Rev, LOW);
}

void moveBackward() {
  digitalWrite(MOTOR_LEFT_Fwd, LOW);
  digitalWrite(MOTOR_LEFT_Rev, HIGH);
  digitalWrite(MOTOR_RIGHT_Fwd, LOW);
  digitalWrite(MOTOR_RIGHT_Rev, HIGH);
}

void turnLeft() {
  digitalWrite(MOTOR_LEFT_Fwd, LOW);
  digitalWrite(MOTOR_LEFT_Rev, HIGH);
  digitalWrite(MOTOR_RIGHT_Fwd, HIGH);
  digitalWrite(MOTOR_RIGHT_Rev, LOW);
}

void turnRight() {
  digitalWrite(MOTOR_LEFT_Fwd, HIGH);
  digitalWrite(MOTOR_LEFT_Rev, LOW);
  digitalWrite(MOTOR_RIGHT_Fwd, LOW);
  digitalWrite(MOTOR_RIGHT_Rev, HIGH);
}

void stopRobot() {
  digitalWrite(MOTOR_LEFT_Fwd, LOW);
  digitalWrite(MOTOR_LEFT_Rev, LOW);
  digitalWrite(MOTOR_RIGHT_Fwd, LOW);
  digitalWrite(MOTOR_RIGHT_Rev, LOW);
}

// ==========================================
// EXTINGUISHING SEQUENCE (Servo & Pump)
// ==========================================

void extinguishFire() {
  Serial.println(F("[FIRE EXTINGUISHER ACTIVATED] Running pump & sweeping nozzle..."));
  
  // Turn on Water Pump
  digitalWrite(WATER_PUMP_PIN, HIGH);
  delay(500); // Wait for water flow to reach nozzle

  // Sweep the servo nozzle to spread water evenly across the fire area
  for (int angle = 50; angle <= 130; angle += 10) {
    nozzleServo.write(angle);
    delay(150);
  }
  for (int angle = 130; angle >= 50; angle -= 10) {
    nozzleServo.write(angle);
    delay(150);
  }
  
  // Reset servo to center
  nozzleServo.write(SERVO_CENTER_POS);
  
  // Turn off water pump
  digitalWrite(WATER_PUMP_PIN, LOW);
  Serial.println(F("[FIRE EXTINGUISHER DEACTIVATED] Pump stopped."));
  
  // Pause to let sensors settle
  delay(1000);
}

// ==========================================
// ULTRASONIC RANGE SENSING (HC-SR04)
// ==========================================

int getDistance() {
  // Clear the trigger pin
  digitalWrite(US_TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Set the trigger pin HIGH for 10 microseconds
  digitalWrite(US_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(US_TRIG_PIN, LOW);
  
  // Read the echo pin (returns sound wave travel time in microseconds)
  long duration = pulseIn(US_ECHO_PIN, HIGH, 30000); // 30ms timeout (~5m max range)
  
  // Calculate distance in cm
  // Speed of sound = 340 m/s or 29.1 microseconds per cm
  // Distance = (Time / 2) / 29.1
  int dist = duration / 58.2;
  
  if (dist <= 0 || dist > 400) {
    return 999; // Return large number if sensor out of range/timeout
  }
  
  return dist;
}
