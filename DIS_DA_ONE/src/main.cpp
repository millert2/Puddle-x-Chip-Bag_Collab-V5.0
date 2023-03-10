/*
  NOTE:
   THIS IS THE STANDARD FOR HOW TO PROPERLY COMMENT CODE
   Header comment has program, name, author name, date created
   Header comment has brief description of what program does
   Header comment has list of key functions and variables created with decription
   There are sufficient in line and block comments in the body of the program
   Variables and functions have logical, intuitive names
   Functions are used to improve modularity, clarity, and readability
***********************************
  RobotIntro.ino
  Kyzer Bowen, Tyce Miller 12.10.22

  This program will introduce using the stepper motor library to create motion algorithms for the robot.
  The motions will be go to angle, go to goal, move in a circle, square, figure eight and teleoperation (stop, forward, spin, reverse, turn)
  It will also include wireless commmunication for remote control of the robot by using a game controller or serial monitor.
  The primary functions created are
  moveCircle - given the diameter in inches and direction of clockwise or counterclockwise, move the robot in a circle with that diameter
  moveFigure8 - given the diameter in inches, use the moveCircle() function with direction input to create a Figure 8
  forward, reverse - both wheels move with same velocity, same direction
  pivot- one wheel stationary, one wheel moves forward or back
  spin - both wheels move with same velocity opposite direction
  turn - both wheels move with same direction different velocity
  stop -both wheels stationary

  Interrupts
  https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
  https://www.arduino.cc/en/Tutorial/CurieTimer1Interrupt
  https://playground.arduino.cc/code/timer1
  https://playground.arduino.cc/Main/TimerPWMCheatsheet
  http://arduinoinfo.mywikis.net/wiki/HOME

  Hardware Connections:
  Arduino pin mappings: https://www.arduino.cc/en/Hacking/PinMapping2560
  A4988 Stepper Motor Driver Pinout: https://www.pololu.com/product/1182 

  digital pin 48 - enable PIN on A4988 Stepper Motor Driver StepSTICK
  digital pin 50 - right stepper motor step pin
  digital pin 51 - right stepper motor direction pin
  digital pin 52 - left stepper motor step pin
  digital pin 53 - left stepper motor direction pin
  digital pin 13 - enable LED on microcontroller

  digital pin 6 - red LED in series with 220 ohm resistor
  digital pin 7 - green LED in series with 220 ohm resistor
  digital pin 8 - yellow LED in series with 220 ohm resistor

  digital pin 18 - left encoder pin
  digital pin 19 - right encoder pin

  digital pin 2 - IMU INT
  digital pin 20 - IMU SDA
  digital pin 21 - IMU SCL


  INSTALL THE LIBRARY
  AccelStepper Library: https://www.airspayce.com/mikem/arduino/AccelStepper/
  
  Sketch->Include Library->Manage Libraries...->AccelStepper->Include
  OR
  Sketch->Include Library->Add .ZIP Library...->AccelStepper-1.53.zip
  See PlatformIO documentation for proper way to install libraries in Visual Studio
*/

#include <Arduino.h>
#include <Accelstepper.h>
#include <MultiStepper.h>
#include <Adafruit_MPU6050.h>
#include <SoftwareSerial.h>

//state LEDs connections
#define redLED 5            //red LED for displaying states
#define grnLED 6            //green LED for displaying states
#define ylwLED 7            //yellow LED for displaying states
#define enableLED 13        //stepper enabled LED

//define motor pin numbers
#define stepperEnable 48    //stepper enable pin on stepStick 
#define rtStepPin 50 //right stepper motor step pin 
#define rtDirPin 51  // right stepper motor direction pin 
#define ltStepPin 52 //left stepper motor step pin 
#define ltDirPin 53  //left stepper motor direction pin 

AccelStepper stepperRight(AccelStepper::DRIVER, rtStepPin, rtDirPin);//create instance of right stepper motor object (2 driver pins, low to high transition step pin 52, direction input pin 53 (high means forward)
AccelStepper stepperLeft(AccelStepper::DRIVER, ltStepPin, ltDirPin);//create instance of left stepper motor object (2 driver pins, step pin 50, direction input pin 51)
MultiStepper steppers;//create instance to control multiple steppers at the same time

#define stepperEnTrue false //variable for enabling stepper motor
#define stepperEnFalse true //variable for disabling stepper motor

int pauseTime = 2500;   //time before robot moves
int stepTime = 500;     //delay time between high and low on step pin
int wait_time = 2000;   //delay for printing data

//define encoder pins
#define LEFT 1        //left encoder
#define RIGHT 0       //right encoder
const int ltEncoder = 18;        //left encoder pin (Mega Interrupt pins 2,3 18,19,20,21)
const int rtEncoder = 19;        //right encoder pin (Mega Interrupt pins 2,3 18,19,20,21)
volatile long encoder[2] = {0, 0};  //interrupt variable to hold number of encoder counts (left, right)
int lastSpeed[2] = {0, 0};          //variable to hold encoder speed (left, right)
int accumTicks[2] = {0, 0};         //variable to hold accumulated ticks since last reset

//define robot measurements
const float wheelDiam = 8.6;    // Wheel diameter on robot (cm)
const float wheelCirc = wheelDiam*PI;    // Wheel circumfrence on robot (cm)
const float robotDiam = 21;   // Robot Diameter from center to center of the wheels (cm)

// define motor velocity 
volatile float veloLeft;
volatile float veloRight;

// define motor error
volatile int errorLeft;
volatile int errorRight;

//IMU object
Adafruit_MPU6050 mpu;

//Bluetooth module connections
#define BTTX 10 // TX on chip to pin 10 on Arduino Mega
#define BTRX 11 //, RX on chip to pin 11 on Arduino Mega
SoftwareSerial BTSerial(BTTX, BTRX);

// Helper Functions

//interrupt function to count left encoder tickes
void LwheelSpeed()
{
  encoder[LEFT] ++;  //count the left wheel encoder interrupts
}

//interrupt function to count right encoder ticks
void RwheelSpeed()
{
  encoder[RIGHT] ++; //count the right wheel encoder interrupts
}

//function to initialize Bluetooth
void init_BT(){
  Serial.println("Goodnight moon!");
  BTSerial.println("Hello, world?");
}
//function to initialize IMU
void init_IMU(){
  Serial.println("Adafruit MPU6050 init!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }
}

//function to set all stepper motor variables, outputs and LEDs
void init_stepper(){
  pinMode(rtStepPin, OUTPUT);//sets pin as output
  pinMode(rtDirPin, OUTPUT);//sets pin as output
  pinMode(ltStepPin, OUTPUT);//sets pin as output
  pinMode(ltDirPin, OUTPUT);//sets pin as output
  pinMode(stepperEnable, OUTPUT);//sets pin as output
  digitalWrite(stepperEnable, stepperEnFalse);//turns off the stepper motor driver
  pinMode(enableLED, OUTPUT);//set enable LED as output
  digitalWrite(enableLED, LOW);//turn off enable LED
  pinMode(redLED, OUTPUT);//set red LED as output
  pinMode(grnLED, OUTPUT);//set green LED as output
  pinMode(ylwLED, OUTPUT);//set yellow LED as output
  digitalWrite(redLED, HIGH);//turn on red LED
  digitalWrite(ylwLED, HIGH);//turn on yellow LED
  digitalWrite(grnLED, HIGH);//turn on green LED
  delay(pauseTime / 5); //wait 0.5 seconds
  digitalWrite(redLED, LOW);//turn off red LED
  digitalWrite(ylwLED, LOW);//turn off yellow LED
  digitalWrite(grnLED, LOW);//turn off green LED

  stepperRight.setMaxSpeed(1500);//set the maximum permitted speed limited by processor and clock speed, no greater than 4000 steps/sec on Arduino
  stepperRight.setAcceleration(10000);//set desired acceleration in steps/s^2
  stepperLeft.setMaxSpeed(1500);//set the maximum permitted speed limited by processor and clock speed, no greater than 4000 steps/sec on Arduino
  stepperLeft.setAcceleration(10000);//set desired acceleration in steps/s^2
  steppers.addStepper(stepperRight);//add right motor to MultiStepper
  steppers.addStepper(stepperLeft);//add left motor to MultiStepper
  digitalWrite(stepperEnable, stepperEnTrue);//turns on the stepper motor driver
  digitalWrite(enableLED, HIGH);//turn on enable LED
}

//function prints encoder data to serial monitor
void print_encoder_data() {
  static unsigned long timer = 0;                           //print manager timer
  if (millis() - timer > 100) {                             //print encoder data every 100 ms or so
    lastSpeed[LEFT] = encoder[LEFT];                        //record the latest left speed value
    lastSpeed[RIGHT] = encoder[RIGHT];                      //record the latest right speed value
    accumTicks[LEFT] = accumTicks[LEFT] + encoder[LEFT];    //record accumulated left ticks
    accumTicks[RIGHT] = accumTicks[RIGHT] + encoder[RIGHT]; //record accumulated right ticks
    Serial.println("Encoder value:");
    Serial.print("\tLeft:\t");
    Serial.print(encoder[LEFT]);
    Serial.print("\tRight:\t");
    Serial.println(encoder[RIGHT]);
    Serial.println("Accumulated Ticks: ");
    Serial.print("\tLeft:\t");
    Serial.print(accumTicks[LEFT]);
    Serial.print("\tRight:\t");
    Serial.println(accumTicks[RIGHT]);
    encoder[LEFT] = 0;                          //clear the left encoder data buffer
    encoder[RIGHT] = 0;                         //clear the right encoder data buffer
    timer = millis();                           //record current time since program started
  }
}

// function resets encoder data
void reset_encoder_data() {
  encoder[LEFT] = 0;                          //clear the left encoder data buffer
  encoder[RIGHT] = 0;                         //clear the right encoder data buffer
}

void update_encoder_data(){

    lastSpeed[LEFT] = encoder[LEFT];                        //record the latest left speed value
    lastSpeed[RIGHT] = encoder[RIGHT];                      //record the latest right speed value
    accumTicks[LEFT] = accumTicks[LEFT] + encoder[LEFT];    //record accumulated left ticks
    accumTicks[RIGHT] = accumTicks[RIGHT] + encoder[RIGHT]; //record accumulated right ticks
}

//function to print IMU data to the serial monitor
void print_IMU_data(){
    /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  /* Print out the values */
  Serial.print("Acceleration X: ");
  Serial.print(a.acceleration.x);
  Serial.print(", Y: ");
  Serial.print(a.acceleration.y);
  Serial.print(", Z: ");
  Serial.print(a.acceleration.z);
  Serial.println(" m/s^2");

  Serial.print("Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");

  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degC");

  Serial.println("");
}

//function to send and receive data with the Bluetooth
void Bluetooth_comm(){
  String data="";
  if (Serial.available()) {
     while (Serial.available()){
      char nextChar = Serial.read();
      data = data + String(nextChar); 
      if (nextChar == ';') {
        break;
      }
     }
    Serial.println(data);
    BTSerial.println(data);
  }
  
  if (BTSerial.available()) {
    while (BTSerial.available()){
      char nextChar = BTSerial.read();
      data = data + String(nextChar); 
      if (nextChar == ';') {
        break;
      }
    }
    Serial.println(data);
    BTSerial.println(data);
  }
}
  
  
/*function to run both wheels to a position at speed*/
void runAtSpeedToPosition() {
  stepperRight.runSpeedToPosition();
  stepperLeft.runSpeedToPosition();
}

/*function to run both wheels continuously at a speed*/
void runAtSpeed ( void ) {
  while (stepperRight.runSpeed() || stepperLeft.runSpeed()) {
  }
}

/*This function, runToStop(), will run the robot until the target is achieved and
   then stop it
*/
void runToStop ( void ) {
  int runNow = 1;
  int rightStopped = 0;
  int leftStopped = 0;

  while (runNow) {
    if (!stepperRight.run()) {
      rightStopped = 1;
      stepperRight.stop();//stop right motor
    }
    if (!stepperLeft.run()) {
      leftStopped = 1;
      stepperLeft.stop();//stop ledt motor
    }
    if (rightStopped && leftStopped) {
      runNow = 0;
    }
  }
}


/*
   The move1() function will move the robot forward one full rotation and backwared on
   full rotation.  Recall that that there 200 steps in one full rotation or 1.8 degrees per
   step. This function uses setting the step pins high and low with delays to move. The speed is set by
   the length of the delay.
*/
void move1() {
  digitalWrite(redLED, HIGH);//turn on red LED
  digitalWrite(grnLED, LOW);//turn off green LED
  digitalWrite(ylwLED, LOW);//turn off yellow LED
  digitalWrite(ltDirPin, HIGH); // Enables the motor to move in a particular direction
  digitalWrite(rtDirPin, HIGH); // Enables the motor to move in a particular direction
  // Makes 800 pulses for making one full cycle rotation
  for (int x = 0; x < 800; x++) {
    digitalWrite(rtStepPin, HIGH);
    digitalWrite(ltStepPin, HIGH);
    delayMicroseconds(stepTime);
    digitalWrite(rtStepPin, LOW);
    digitalWrite(ltStepPin, LOW);
    delayMicroseconds(stepTime);
  }
  delay(1000); // One second delay
  digitalWrite(ltDirPin, LOW); // Enables the motor to move in opposite direction
  digitalWrite(rtDirPin, LOW); // Enables the motor to move in opposite direction
  // Makes 800 pulses for making one full cycle rotation
  for (int x = 0; x < 800; x++) {
    digitalWrite(rtStepPin, HIGH);
    digitalWrite(ltStepPin, HIGH);
    delayMicroseconds(stepTime);
    digitalWrite(rtStepPin, LOW);
    digitalWrite(ltStepPin, LOW);
    delayMicroseconds(stepTime);
  }
  delay(1000); // One second delay
}

/*
   The move2() function will use AccelStepper library functions to move the robot
   move() is a library function for relative movement to set a target position
   moveTo() is a library function for absolute movement to set a target position
   stop() is a library function that causes the stepper to stop as quickly as possible
   run() is a library function that uses accel and decel to achieve target position, no blocking
   runSpeed() is a library function that uses constant speed to achieve target position, no blocking
   runToPosition() is a library function that uses blocking with accel/decel to achieve target position
   runSpeedToPosition() is a library function that uses constant speed to achieve target posiiton, no blocking
   runToNewPosition() is a library function that uses blocking with accel/decel to achieve target posiiton
*/
void move2() {
  digitalWrite(redLED, LOW);//turn off red LED
  digitalWrite(grnLED, HIGH);//turn on green LED
  digitalWrite(ylwLED, LOW);//turn off yellow LED
  stepperRight.moveTo(800);//move one full rotation forward relative to current position
  stepperLeft.moveTo(800);//move one full rotation forward relative to current position
  stepperRight.setSpeed(1000);//set right motor speed
  stepperLeft.setSpeed(1000);//set left motor speed
  stepperRight.runSpeedToPosition();//move right motor
  stepperLeft.runSpeedToPosition();//move left motor
  runToStop();//run until the robot reaches the target
  delay(1000); // One second delay
  stepperRight.moveTo(0);//move one full rotation backward relative to current position
  stepperLeft.moveTo(0);//move one full rotation backward relative to current position
  stepperRight.setSpeed(1000);//set right motor speed
  stepperLeft.setSpeed(1000);//set left motor speed
  stepperRight.runSpeedToPosition();//move right motor
  stepperLeft.runSpeedToPosition();//move left motor
  runToStop();//run until the robot reaches the target
  delay(1000); // One second delay
}

/*
   The move3() function will use the MultiStepper() class to move both motors at once
   move() is a library function for relative movement to set a target position
   moveTo() is a library function for absolute movement to set a target position
   stop() is a library function that causes the stepper to stop as quickly as possible
   run() is a library function that uses accel and decel to achieve target position, no blocking
   runSpeed() is a library function that uses constant speed to achieve target position, no blocking
   runToPosition() is a library function that uses blocking with accel/decel to achieve target position
   runSpeedToPosition() is a library function that uses constant speed to achieve target posiiton, no blocking
   runToNewPosition() is a library function that uses blocking with accel/decel to achieve target posiiton
*/
void move3() {
  digitalWrite(redLED, LOW);//turn off red LED
  digitalWrite(grnLED, LOW);//turn off green LED
  digitalWrite(ylwLED, HIGH);//turn on yellow LED
  long positions[2]; // Array of desired stepper positions
  positions[0] = 800;//right motor absolute position
  positions[1] = 800;//left motor absolute position
  steppers.moveTo(positions);
  steppers.runSpeedToPosition(); // Blocks until all are in position
  delay(1000);//wait one second
  // Move to a different coordinate
  positions[0] = 0;//right motor absolute position
  positions[1] = 0;//left motor absolute position
  steppers.moveTo(positions);
  steppers.runSpeedToPosition(); // Blocks until all are in position
  delay(1000);//wait one second
}

/*this function will move to target at 2 different speeds*/
void move4() {

  int leftPos = 5000;//right motor absolute position
  int rightPos = 1000;//left motor absolute position
  int leftSpd = 5000;//right motor speed
  int rightSpd = 1000; //left motor speed

  digitalWrite(redLED, HIGH);//turn on red LED
  digitalWrite(grnLED, HIGH);//turn on green LED
  digitalWrite(ylwLED, LOW);//turn off yellow LED

  //Uncomment the next 4 lines for absolute movement
  stepperLeft.setCurrentPosition(0);//set left wheel position to zero
  stepperRight.setCurrentPosition(0);//set right wheel position to zero
  stepperLeft.moveTo(leftPos);//move left wheel to absolute position
  stepperRight.moveTo(rightPos);//move right wheel to absolute position

  //Unomment the next 2 lines for relative movement
  stepperLeft.move(leftPos);//move left wheel to relative position
  stepperRight.move(rightPos);//move right wheel to relative position

  //Uncomment the next two lines to set the speed
  stepperLeft.setSpeed(leftSpd);//set left motor speed
  stepperRight.setSpeed(rightSpd);//set right motor speed
  runAtSpeedToPosition();//run at speed to target position
}

/*This function will move continuously at 2 different speeds*/
void move5() {
  digitalWrite(redLED, LOW);//turn off red LED
  digitalWrite(grnLED, HIGH);//turn on green LED
  digitalWrite(ylwLED, HIGH);//turn on yellow LED
  int leftSpd = 5000;//right motor speed
  int rightSpd = 1000; //left motor speed
  stepperLeft.setSpeed(leftSpd);//set left motor speed
  stepperRight.setSpeed(rightSpd);//set right motor speed
  runAtSpeed();
}




/*
  Pivots the robot in a given direction by stopping one motor and driving the other
*/
void pivot(int direction) {
  int wheelStepsForDistance = (800 / wheelCirc) * ((robotDiam * PI) / 2); // (steps per rotation / distance per rotation) * desired distance

  if (direction == 0){
    stepperRight.moveTo(wheelStepsForDistance);
    stepperLeft.moveTo(0);
    stepperRight.setMaxSpeed(300);//set right motor speed
    stepperLeft.setMaxSpeed(300);//set left motor speed
    stepperRight.runSpeedToPosition();//move right motor
    stepperLeft.runSpeedToPosition();//move left motor
    runToStop();//run until the robot reaches the target
  } else {
    stepperRight.moveTo(0);
    stepperLeft.moveTo(wheelStepsForDistance);
    stepperRight.setMaxSpeed(300);//set right motor speed
    stepperLeft.setMaxSpeed(300);//set left motor speed
    stepperRight.runSpeedToPosition();//move right motor
    stepperLeft.runSpeedToPosition();//move left motor
    runToStop();//run until the robot reaches the target
  }
    stepperRight.setCurrentPosition(0); // Resets stepper motor position to 0
    stepperLeft.setCurrentPosition(0); // Resets stepper motor position to 0
  
}

/*
  The robot spins in a given direction for a given angle. The two wheels run at equal and opposite velocities
*/
void spin(int direction, int angle) {

  float angle_rad = ((angle) * PI) / 180; // Converts input angle to radians

 
  
  float wheelStepsForDistance = (800 / wheelCirc) * (robotDiam * (angle_rad)/2); // (steps per rotation / distance per rotation) * desired distance

  // Calculates the distance in encoder ticks for both motors
  float desiredEncoderTicks = (angle / 3.68); // 3.68 is degrees per pulse for the size of our wheels;
 

  // Calculates the steps needed from encoders
  float stepsFromEncoder = ((desiredEncoderTicks / 40) * 800);



  stepperRight.setCurrentPosition(0); // Resets stepper motor position to 0
  stepperLeft.setCurrentPosition(0);  // Resets stepper motor position to 0

  if (direction == 1){
    stepperRight.moveTo(stepsFromEncoder); //set motor steps
    stepperLeft.moveTo(-stepsFromEncoder);  //set motor steps
    stepperRight.setMaxSpeed(300);//set right motor speed
    stepperLeft.setMaxSpeed(300);//set left motor speed
    stepperRight.runSpeedToPosition();//move right motor
    stepperLeft.runSpeedToPosition();//move left motor
    runToStop();//run until the robot reaches the target

    


    errorLeft = (800/40)*(desiredEncoderTicks - encoder[LEFT]); // Calculates error and adjusts left motor
    errorRight =(800/40)*(desiredEncoderTicks - encoder[RIGHT]);  // Calculates error and adjusts right motor

        
    stepperRight.setCurrentPosition(0); // Resets stepper motor position to 0
    stepperLeft.setCurrentPosition(0);  // Resets stepper motor position to 0
    stepperRight.setMaxSpeed(300);//set right motor speed
    stepperLeft.setMaxSpeed(300);//set left motor speed
    stepperRight.moveTo(errorRight);  // Moves motor to correct for error
    stepperLeft.moveTo(-errorLeft); // Moves motor to correct for error
    runToStop();//run until the robot reaches the target

    print_encoder_data(); // Used to troubleshoot and check encoders

  } else {
    stepperRight.moveTo(-stepsFromEncoder); //set motor steps
    stepperLeft.moveTo(stepsFromEncoder); //set motor steps 
    stepperRight.setMaxSpeed(300);//set right motor speed
    stepperLeft.setMaxSpeed(300);//set left motor speed
    stepperRight.runSpeedToPosition();//move right motor
    stepperLeft.runSpeedToPosition();//move left motor
    runToStop();//run until the robot reaches the target

    

    Serial.println("Checking for error.....");  // Used to troubleshoot
    delay(1000);
    
    errorLeft = (800/40)*(desiredEncoderTicks - encoder[LEFT]); // Calculates error and adjusts left motor
    errorRight = (800/40)*(desiredEncoderTicks - encoder[RIGHT]); // Calculates error and adjusts left motor

        
    stepperRight.setCurrentPosition(0); // Resets stepper motor position to 0
    stepperLeft.setCurrentPosition(0);  // Resets stepper motor position to 0
    stepperRight.setMaxSpeed(300);  //set right motor speed
    stepperLeft.setMaxSpeed(300); //set left motor speed
    stepperRight.moveTo(-errorRight);
    stepperLeft.moveTo(errorLeft);
    runToStop();//run until the robot reaches the target

    print_encoder_data(); // Used to troubleshoot and check encoders

    
    
  }
  stepperRight.setCurrentPosition(0); // Resets stepper motor position to 0
  stepperLeft.setCurrentPosition(0);  // Resets stepper motor position to 0

    
  
}

/*
  Turns the robot based off the input direction. The robot turns at a fixed radius
*/
void turn(int direction) {
    int wheelStepsForDistance = (800 / wheelCirc) * ((robotDiam * PI)/2); // (steps per rotation / distance per rotation) * desired distance
  
    if (direction == 0){
    stepperRight.moveTo(wheelStepsForDistance); // Moves stepper
    stepperLeft.moveTo(wheelStepsForDistance/2); // Moves stepper 
    stepperRight.setMaxSpeed(300);//set right motor speed
    stepperLeft.setMaxSpeed(150);//set left motor speed
    stepperRight.runSpeedToPosition();//move right motor
    stepperLeft.runSpeedToPosition();//move left motor
    runToStop();//run until the robot reaches the target
  } else {
    stepperRight.moveTo(wheelStepsForDistance/2);// Moves stepper
    stepperLeft.moveTo(wheelStepsForDistance);// Moves stepper
    stepperRight.setMaxSpeed(150);//set right motor speed
    stepperLeft.setMaxSpeed(300);//set left motor speed
    stepperRight.runSpeedToPosition();//move right motor
    stepperLeft.runSpeedToPosition();//move left motor
    runToStop();//run until the robot reaches the target
  }
    stepperRight.setCurrentPosition(0); // Resets stepper motor position to 0
    stepperLeft.setCurrentPosition(0);  // Resets stepper motor position to 0
  
}
/*
  Moves the robot in the forward direction for a given distance
*/
void forward(int distance) {
  
  // Calculates the distance in cm to wheel steps  (800  steps per rotation)
  float wheelStepsForDistance = (800 / wheelCirc) * distance; // (steps per rotation / distance per rotation) * desired distance

  // Calculates the distance in encoder ticks for both motors
  float desiredEncoderTicks = (40 / wheelCirc) * distance;
  Serial.println(desiredEncoderTicks);// Used to troubleshoot

  // Calculates the steps needed from encoders
  float stepsFromEncoder = (desiredEncoderTicks / 40) * 800;

  Serial.println(stepsFromEncoder);// Used to troubleshoot

  stepperRight.setCurrentPosition(0); // Resets stepper motor position to 0
  stepperLeft.setCurrentPosition(0);  // Resets stepper motor position to 0
  stepperRight.setMaxSpeed(300);//set right motor speed
  stepperLeft.setMaxSpeed(300);//set left motor speed
  stepperRight.moveTo(stepsFromEncoder);
  stepperLeft.moveTo(stepsFromEncoder);
  runToStop();//run until the robot reaches the target


  errorLeft = desiredEncoderTicks - encoder[LEFT];
  errorRight = desiredEncoderTicks - encoder[RIGHT];

  Serial.println(errorLeft);  // Used to troubleshoot
  Serial.println(errorRight);// Used to troubleshoot
  
  
  stepperRight.setCurrentPosition(0);
  stepperLeft.setCurrentPosition(0);
  stepperRight.setMaxSpeed(300);//set right motor speed
  stepperLeft.setMaxSpeed(300);//set left motor speed
  stepperRight.moveTo(errorRight);
  stepperLeft.moveTo(errorLeft);
  runToStop();//run until the robot reaches the target



  print_encoder_data();// Used to troubleshoot

}


  
/*
  Moves the robot in the backwards direction for a given distance
*/
void reverse(int distance) {
  // Calculates the distance in inches to wheel steps  (200  steps per rotation)
  int wheelStepsForDistance = (800 / wheelCirc) * distance; // (steps per rotation / distance per rotation) * desired distance

  // Moves both motors to desired distance
 /* long positions[2]; // Array of desired stepper positions
  positions[0] = -wheelStepsForDistance;//right motor absolute position
  positions[1] = -wheelStepsForDistance;//left motor absolute position
  steppers.moveTo(positions);
  steppers.runSpeedToPosition(); // Blocks until all are in position\\  stepperRight.moveTo(wheelStepsForDistance);
  stepperLeft.moveTo(wheelStepsForDistance); */

  stepperRight.moveTo(0); // Resets stepper motor position to 0
  stepperLeft.moveTo(0);  // Resets stepper motor position to 0
  stepperRight.setMaxSpeed(500);//set right motor speed
  stepperLeft.setMaxSpeed(500);//set left motor speed
  stepperRight.runSpeedToPosition();//move right motor
  stepperLeft.runSpeedToPosition();//move left motor
  runToStop();//run until the robot reaches the target
}
/*
   Stops the robot
*/
void stop() {
  stepperRight.setSpeed(0);//set right motor speed
  stepperLeft.setSpeed(0);//set left motor speed
  stepperLeft.stop();//stop left motor
  stepperRight.stop();//stop right motor
  
}




/*
  moves the robot in a full circle based off a given diameter and direction
*/
void moveCircle(int diam, int dir) {

  // Geometry Calculations needed for stepper motor position
  int innerDiam = diam - robotDiam; // Inner diameter calculation of inner wheel to circle
  int outterDiam = diam + robotDiam;  // Outter diameter calculation of outer wheel to circle

  int innerCirc = innerDiam * (PI); // Circum of the Inner Circle by the Inner Wheel
  int outterCirc = outterDiam * (PI);  // Circum of the Outter Circle by the Outter Wheel

  float innerTicks = (800 / wheelCirc) * (innerCirc); // (steps per rotation / distance per rotation) * cgghdesired distance
  float outterTicks = (800 / wheelCirc) * (outterCirc); // (steps per rotation / distance per rotation) * desired distance
  float circleFactor = innerTicks / outterTicks;  // Makes velocity proportional for the amount of ticks each wheel has to go

  float outterSpeed = 500;  //  Speed of outter wheel
  float innerSpeed = circleFactor * 500;  // Speed of inner wheel

  digitalWrite(redLED, HIGH); // Turns redlight on


  
  if (dir == 0){
    stepperRight.moveTo(innerTicks);  // Moves Right Stepper motor to desired amount of ticks
    stepperLeft.moveTo(outterTicks);  // Moves Right Stepper motor to desired amount of ticks
    stepperRight.setMaxSpeed(innerSpeed);//set right motor speed
    stepperLeft.setMaxSpeed(outterSpeed);//set left motor speed
    stepperRight.setSpeed(innerSpeed);//set right motor speed
    stepperLeft.setSpeed(outterSpeed);//set left motor speed
    stepperRight.runSpeedToPosition();//move right motor
    stepperLeft.runSpeedToPosition();//move left motor
    runToStop();//run until the robot reaches the target

    stepperRight.setCurrentPosition(0); // Resets Stepper postion
    stepperLeft.setCurrentPosition(0);  // Resets Stepper postion
    
  }

  else{
    stepperRight.moveTo(outterTicks); // Moves Right Stepper motor to desired amount of ticks
    stepperLeft.moveTo(innerTicks); // Moves Right Stepper motor to desired amount of ticks
    stepperRight.setMaxSpeed(outterSpeed);//set right motor speed
    stepperLeft.setMaxSpeed(innerSpeed);//set left motor speed
    stepperRight.setSpeed(outterSpeed);//set right motor speed
    stepperLeft.setSpeed(innerSpeed);//set left motor speed
    stepperRight.runSpeedToPosition();//move right motor
    stepperLeft.runSpeedToPosition();//move left motor
    runToStop();//run until the robot reaches the target

    stepperRight.setCurrentPosition(0); // Resets Stepper postion
    stepperLeft.setCurrentPosition(0);  // Resets Stepper postion
    
  }

  digitalWrite(redLED, LOW);  // Turns red light off
  
}

/*
  The moveFigure8() function takes the diameter in inches as the input. It uses the moveCircle() function
  twice with 2 different direcitons to create a figure 8 with circles of the given diameter.
*/
void moveFigure8(int diam) {

  digitalWrite(redLED, HIGH);// Turns red light on
  digitalWrite(ylwLED, HIGH);// Turns ylw light on

  moveCircle(diam,0); // Circle Left
  digitalWrite(redLED, HIGH); // Turns red light on
  
  moveCircle(diam,1); // Circle Right

  digitalWrite(redLED, LOW);// Turns red light off
  digitalWrite(ylwLED, LOW);// Turns ylw light off

  
}

void goToAngle(int angle){
  stepperRight.setCurrentPosition(0); // Resets Stepper postion
  stepperLeft.setCurrentPosition(0);  // Resets Stepper postion

  if (angle > 0){
    spin(0,angle);  // Spins fastest direction to angle
  }
  else{
    angle = abs(angle); // Changes sign for angle to be inputed into spin function
    spin(1,angle); // Spins fastest direction to angle
  }

}

void goToGoal(float x, float y){
  float angle = atan2(y,x) * (180/PI);  // Calculates angle in degrees robot needs to turn
  goToAngle(angle); // Turns robot to neccessary angle

  x = abs(x); // Change to a positive number for distance calc
  y = abs(y);// Change to a positive number for distance calc
  
  float distance = sqrt((x*x) + (y*y)); // Distance calculation for go to goal
  
  forward(distance);  // moves robot to goal point 
  
  
}

void makeSquare(int side_length){
  
  for (int j = 0; j < 4; j++){
    forward(side_length); // Moves the robot on the sides of the square
    spin(0,90); // Spins the robot 90 deg
  }
}

// Troubleshooting function that was often used 
void testEncoders(){
  print_encoder_data(); // Troubleshooting

  stepperRight.setCurrentPosition(0); // Resets motor position
  stepperLeft.setCurrentPosition(0);// Resets motor position
  stepperRight.setMaxSpeed(300);//set right motor speed
  stepperLeft.setMaxSpeed(300);//set left motor speed
  stepperRight.moveTo(200); // New motor position
  stepperLeft.moveTo(200);// New motor position
  runToStop();//run until the robot reaches the target

  
  print_encoder_data();// Troubleshooting
  delay(1000);

  stepperRight.setCurrentPosition(0);// Resets motor position
  stepperLeft.setCurrentPosition(0);// Resets motor position
  stepperRight.setMaxSpeed(300);//set right motor speed
  stepperLeft.setMaxSpeed(300);//set left motor speed
  stepperRight.moveTo(400);// New motor position
  stepperLeft.moveTo(400);// New motor position
  runToStop();//run until the robot reaches the target

  print_encoder_data();// Troubleshooting
  delay(1000);


  stepperRight.setCurrentPosition(0);// Resets motor position
  stepperLeft.setCurrentPosition(0);// Resets motor position
  stepperRight.setMaxSpeed(300);//set right motor speed
  stepperLeft.setMaxSpeed(300);//set left motor speed
  stepperRight.moveTo(800);// New motor position
  stepperLeft.moveTo(800);// New motor position
  runToStop();//run until the robot reaches the target

  print_encoder_data();// Troubleshooting
  delay(1000);
    

  stepperRight.setCurrentPosition(0);// Resets motor position
  stepperLeft.setCurrentPosition(0);// Resets motor position
  stepperRight.setMaxSpeed(300);//set right motor speed
  stepperLeft.setMaxSpeed(300);//set left motor speed
  stepperRight.moveTo(1600);// New motor position
  stepperLeft.moveTo(1600);// New motor position
  runToStop();//run until the robot reaches the target

  print_encoder_data();// Troubleshooting
}


//// MAIN
void setup()
{
  delay(5000);
  int baudrate = 9600; //serial monitor baud rate'
  int BTbaud = 9600;  // HC-05 default speed in AT command more
  init_stepper(); //set up stepper motor

  attachInterrupt(digitalPinToInterrupt(ltEncoder), LwheelSpeed, CHANGE);    //init the interrupt mode for the left encoder
  attachInterrupt(digitalPinToInterrupt(rtEncoder), RwheelSpeed, CHANGE);   //init the interrupt mode for the right encoder

  //BTSerial.begin(BTbaud);     //start Bluetooth communication
  Serial.begin(baudrate);     //start serial monitor communication

  while (!Serial)
    delay(10); // will pause until serial console opens
  
  //init_BT(); //initialize Bluetooth

  //init_IMU(); //initialize IMU
  
  Serial.println("Robot starting...");
  Serial.println("");
  delay(pauseTime); //always wait 2.5 seconds before the robot moves
}


void loop()
{
 

  //uncomment each function one at a time to see what the code does
  //move1();//call move back and forth function
  //move2();//call move back and forth function with AccelStepper library functions
  //move3();//call move back and forth function with MultiStepper library functions
  //move4(); //move to target position with 2 different speeds
  //move5(); //move continuously with 2 different speeds

  //Uncomment to read Encoder Data (uncomment to read on serial monitor)
  //print_encoder_data();   //prints encoder data

  //Uncomment to read IMU Data (uncomment to read on serial monitor)
  //print_IMU_data();         //print IMU data

  //Uncomment to Send and Receive with Bluetooth
  //Bluetooth_comm();

  delay(wait_time);               //wait to move robot or read data
}