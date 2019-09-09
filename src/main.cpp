#include <main.h>

void setup() {
  // start serial
  SerialUSB.begin(115200);

  delay(50);
  SerialUSB.println("Initializing...");
  
  // setup the M2 LEDs and buttons
  setupLightsAndButtons();

  // setup HID Joystick
  setupJoystick();

  // start SPI communication to the MCP4261
  setupPotentiometers();

  // setup CAN
  setupCAN();

  // change mode when button is pressed
  attachInterrupt(Button2, toggleEmulationMode, FALLING);
}

void loop() {
  if (can.newVehicleData()) {
    updatePose(can.pose);
  }
}

void setupLightsAndButtons() {
  pinMode(DS2, OUTPUT);
  pinMode(DS3, OUTPUT);
  pinMode(DS4, OUTPUT);
  pinMode(DS5, OUTPUT);
  pinMode(DS6, OUTPUT);
  pinMode(DS7_RED, OUTPUT);
  pinMode(DS7_GREEN, OUTPUT);
  pinMode(DS7_BLUE, OUTPUT);

  pinMode(Button1, INPUT_PULLUP);
  pinMode(Button2, INPUT_PULLUP);

  digitalWrite(DS2, HIGH);
  digitalWrite(DS3, HIGH);
  digitalWrite(DS4, HIGH);
  digitalWrite(DS5, HIGH);
  digitalWrite(DS6, HIGH);
  digitalWrite(DS7_RED, HIGH);
  digitalWrite(DS7_GREEN, LOW);
  digitalWrite(DS7_BLUE, HIGH);
}

void setupJoystick() {
  if (joystick != nullptr)
    joystick->end();
    
  switch (mode) {
    case EmulationMode::Xbox:
      joystick = new Joystick_(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD, 8, 0, true, false, false, false, false, false, false, false, false, false, false);
      joystick->setXAxisRange(-900, 900);
    break;
    case EmulationMode::PC:
      joystick = new Joystick_(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD, 8, 0, false, false, false, false, false, false, false, false, true, true, true);
      joystick->setSteeringRange(-900, 900);
      joystick->setThrottleRange(0, 255);
      joystick->setBrakeRange(0, 100);
    break;
  }

  joystick->begin(false);
}

void setupPotentiometers() {
  SPI.begin();
  pot.setResistance(0, 0);
  pot.setResistance(1, 0);
  pot.scale = 255.0;
}

void setupCAN() {
  digitalWrite(DS6, LOW);
  SerialUSB.println("Doing Auto Baud scan on CAN0");
  Can0.beginAutoSpeed();
  SerialUSB.println("Doing Auto Baud scan on CAN1");
  Can1.beginAutoSpeed();
  digitalWrite(DS7_BLUE, LOW);

  int filter;
  //extended
  for (filter = 0; filter < 3; filter++) {
    Can0.setRXFilter(filter, 0, 0, true);
    Can1.setRXFilter(filter, 0, 0, true);
  }  
  //standard
  for (int filter = 3; filter < 7; filter++) {
    Can0.setRXFilter(filter, 0, 0, false);
    Can1.setRXFilter(filter, 0, 0, false);
  }
}

void updatePose(Pose pose) {
  switch (mode) {
    case EmulationMode::Xbox:
      // port 0 = left trigger
      pot.setResistance(0, pose.brakes);

      // port 1 = right trigger
      pot.setResistance(1, pose.accelerator);

      // steering
      joystick->setXAxis(pose.steering);
      break;
    case EmulationMode::PC:
      // steering, accelerator, and brakes
      joystick->setSteering(pose.steering);
      joystick->setAccelerator(pose.accelerator);
      joystick->setBrake(pose.brakes);
      break;
  }

  // clutch
  pose.clutch ? joystick->pressButton(XboxButtons::LEFT_BUMPER) : joystick->releaseButton(XboxButtons::LEFT_BUMPER);

  // e-brake
  pose.ebrake ? joystick->pressButton(XboxButtons::A) : joystick->releaseButton(XboxButtons::A);

  // upshift
  pose.upshift ? joystick->pressButton(XboxButtons::B) : joystick->releaseButton(XboxButtons::B);

  // downshift: remap X1 on Adaptive Controller to X
  pose.downshift ? joystick->pressButton(XboxButtons::LEFT_X1) : joystick->releaseButton(XboxButtons::LEFT_X1);

  // rewind: remap X2 on Adaptive Controller to Y
  pose.rewind ? joystick->pressButton(XboxButtons::LEFT_X2) : joystick->releaseButton(XboxButtons::LEFT_X2);

  // update joystick state
  joystick->sendState();
}

void toggleEmulationMode() {
  switch (mode) {
    case EmulationMode::Xbox:
      mode = EmulationMode::PC;
      digitalWrite(DS7_GREEN, LOW);
      digitalWrite(DS7_BLUE, HIGH);

      setupJoystick();
      break;
    case EmulationMode::PC:
      mode = EmulationMode::Xbox;
      digitalWrite(DS7_GREEN, HIGH);
      digitalWrite(DS7_BLUE, LOW);

      setupJoystick();
      break;
  }
}