/*
  Title:  B37VB Conga Line Example
  Author: Kieran O'Leary  
  Modified: Added SEEK LED and LDR ADC spike duration printing
*/

#include <stdint.h>

#define FIRMWARE_VERSION "v1.0.1"

#define UART_BAUDRATE 115200 // Baud Rate for UART interface

#define LOCK_INDICATOR_PIN       4
#define MOTOR_L1_PIN             5
#define MOTOR_L2_PIN             6
#define TRANSMIT_ID_CONTROL_PIN  7
#define MOTOR_R1_PIN             9
#define MOTOR_R2_PIN            10
#define BUGGY_ID_PIN_0           8
#define BUGGY_ID_PIN_1          11
#define BUGGY_ID_PIN_2          12
#define BUGGY_ID_PIN_3          13
#define LDR_L_PIN               A0
#define LDR_R_PIN               A1
#define SEEK_INDICATOR_PIN       3 // LED indicates buggy is seeking another buggy

#define ADC_RESOLUTION         10
#define ADC_OUTPUT_RANGE       (1 << ADC_RESOLUTION)
#define ADC_FULL_SCALE_VOLTAGE 5.0

#define BUGGY_ID_LEADER_MODE 0

#define LDR_LO_LEVEL_THRESHOLD    350
#define LDR_HI_LEVEL_THRESHOLD    500
#define LDR_STOP_LEVEL_THRESHOLD 1000
#define LDR_PAIR_TURN_THRESHOLD   100
#define LDR_SPIKE_THRESHOLD       600

#define CONGA_LINE_MAX_BUGGIES 2

#define BUGGY_ID_PULSE_ON_TIME_STEP_SIZE_MS 400
#define BUGGY_ID_PULSE_PERIOD_MS (CONGA_LINE_MAX_BUGGIES * BUGGY_ID_PULSE_ON_TIME_STEP_SIZE_MS * 1)
#define BUGGY_ID_PULSE_DETECT_PERIOD_MARGIN_MS    100
#define BUGGY_ID_PULSE_DETECT_ON_TIME_MARGIN_MS   20
#define BUGGY_ID_PULSE_DETECT_VALID_PULSE_MIN_NUM 3
#define BUGGY_ID_PULSE_DETECT_DELAY_MS            (BUGGY_ID_PULSE_PERIOD_MS - BUGGY_ID_PULSE_DETECT_PERIOD_MARGIN_MS)
#define BUGGY_ID_PULSE_DETECT_NO_PULSE_TIMEOUT_MS (BUGGY_ID_PULSE_PERIOD_MS + BUGGY_ID_PULSE_DETECT_PERIOD_MARGIN_MS)

#define MOTOR_L_FORWARD_PWM_VALUE     150
#define MOTOR_R_FORWARD_PWM_VALUE     150
#define MOTOR_L_REVERSE_PWM_VALUE    -150
#define MOTOR_R_REVERSE_PWM_VALUE    -150
#define MOTOR_L_LEFT_TURN_PWM_VALUE   100
#define MOTOR_R_LEFT_TURN_PWM_VALUE   200
#define MOTOR_L_RIGHT_TURN_PWM_VALUE  200
#define MOTOR_R_RIGHT_TURN_PWM_VALUE  100

#define LIGHT_TRACKING_LEADER_MODE_UPDATE_PERIOD_MS   100
#define LIGHT_TRACKING_FOLLOWER_MODE_UPDATE_PERIOD_MS 100
#define LIGHT_TRACKING_FOLLOWER_MODE_SEEK_PATTERN 0

uint8_t BuggyID;

typedef enum
{
  PULSE_DETECT_FSM_INIT,
  PULSE_DETECT_FSM_DETECT_START_CONDITION,
  PULSE_DETECT_FSM_DETECT_PULSE_START,
  PULSE_DETECT_FSM_DETECT_PULSE_END,
  PULSE_DETECT_FSM_DISABLE_PULSE_START_DETECTION
} PulseDetectFSM_StateTypeDef;

// ---------------- Functions ----------------

// Measure LDR voltage
uint16_t MeasureLDRCircuitVoltage(int PinNumber)
{
  return analogRead(PinNumber);
}

// Detect Buggy ID pulses
uint8_t DetectBuggyIDPulses()
{
  static PulseDetectFSM_StateTypeDef FSMState;
  static uint16_t ValidPulseCount = 0;
  static uint8_t LockState = 0;
  static uint32_t ValidPulseWidthNom, ValidPulseWidthMin, ValidPulseWidthMax;
  static uint32_t PulseStartTimestamp, PulseStopTimestamp;
  uint32_t CurrentTimestamp;
  uint16_t LeftLDRValue, RightLDRValue;
  uint32_t MeasuredPulseWidth;

  switch (FSMState)
  {
    case PULSE_DETECT_FSM_INIT:
      ValidPulseCount = 0;
      LockState = 0;
      digitalWrite(LOCK_INDICATOR_PIN, LOW);
      ValidPulseWidthNom = BuggyID * BUGGY_ID_PULSE_ON_TIME_STEP_SIZE_MS;
      ValidPulseWidthMin = ValidPulseWidthNom - BUGGY_ID_PULSE_DETECT_ON_TIME_MARGIN_MS;
      ValidPulseWidthMax = ValidPulseWidthNom + BUGGY_ID_PULSE_DETECT_ON_TIME_MARGIN_MS;
      PulseStartTimestamp = PulseStopTimestamp = 0;
      FSMState = PULSE_DETECT_FSM_DETECT_START_CONDITION;
      break;

    case PULSE_DETECT_FSM_DETECT_START_CONDITION:
      LeftLDRValue = MeasureLDRCircuitVoltage(LDR_L_PIN);
      RightLDRValue = MeasureLDRCircuitVoltage(LDR_R_PIN);
      if (LeftLDRValue < LDR_LO_LEVEL_THRESHOLD && RightLDRValue < LDR_LO_LEVEL_THRESHOLD)
        FSMState = PULSE_DETECT_FSM_DETECT_PULSE_START;
      break;

    case PULSE_DETECT_FSM_DETECT_PULSE_START:
      LeftLDRValue = MeasureLDRCircuitVoltage(LDR_L_PIN);
      RightLDRValue = MeasureLDRCircuitVoltage(LDR_R_PIN);
      CurrentTimestamp = millis();
      if (LeftLDRValue > LDR_HI_LEVEL_THRESHOLD || RightLDRValue > LDR_HI_LEVEL_THRESHOLD)
      {
        PulseStartTimestamp = millis();
        FSMState = PULSE_DETECT_FSM_DETECT_PULSE_END;
      }
      else if (ValidPulseCount > 0 && CurrentTimestamp - PulseStartTimestamp > BUGGY_ID_PULSE_DETECT_NO_PULSE_TIMEOUT_MS)
        FSMState = PULSE_DETECT_FSM_INIT;
      break;

    case PULSE_DETECT_FSM_DETECT_PULSE_END:
      LeftLDRValue = MeasureLDRCircuitVoltage(LDR_L_PIN);
      RightLDRValue = MeasureLDRCircuitVoltage(LDR_R_PIN);
      if (LeftLDRValue < LDR_LO_LEVEL_THRESHOLD && RightLDRValue < LDR_LO_LEVEL_THRESHOLD)
      {
        PulseStopTimestamp = millis();
        MeasuredPulseWidth = PulseStopTimestamp - PulseStartTimestamp;
        if (MeasuredPulseWidth >= ValidPulseWidthMin && MeasuredPulseWidth <= ValidPulseWidthMax)
        {
          ValidPulseCount++;
          if (ValidPulseCount == BUGGY_ID_PULSE_DETECT_VALID_PULSE_MIN_NUM)
          {
            LockState = 1;
            digitalWrite(LOCK_INDICATOR_PIN, HIGH);
          }
          FSMState = PULSE_DETECT_FSM_DISABLE_PULSE_START_DETECTION;
        }
        else FSMState = PULSE_DETECT_FSM_INIT;
      }
      else if (millis() - PulseStartTimestamp > ValidPulseWidthMax)
        FSMState = PULSE_DETECT_FSM_INIT;
      break;

    case PULSE_DETECT_FSM_DISABLE_PULSE_START_DETECTION:
      if (millis() - PulseStartTimestamp > BUGGY_ID_PULSE_DETECT_DELAY_MS)
        FSMState = PULSE_DETECT_FSM_DETECT_PULSE_START;
      break;

    default:
      FSMState = PULSE_DETECT_FSM_INIT;
      break;
  }

  return LockState;
}

// Transmit Buggy ID pulses
void TransmitBuggyIDPulses()
{
  static uint32_t PreviousTimestamp = millis();
  static uint8_t LedEnablePinState = LOW;
  uint32_t CurrentTimestamp = millis();

  const uint32_t LedOnDurationMSec  = (BuggyID + 1) * BUGGY_ID_PULSE_ON_TIME_STEP_SIZE_MS;
  const uint32_t LedOffDurationMSec = BUGGY_ID_PULSE_PERIOD_MS - LedOnDurationMSec;

  if (LedEnablePinState == LOW && CurrentTimestamp - PreviousTimestamp >= LedOffDurationMSec)
  {
    digitalWrite(TRANSMIT_ID_CONTROL_PIN, HIGH);
    LedEnablePinState = HIGH;
    PreviousTimestamp = CurrentTimestamp;
  }
  else if (LedEnablePinState == HIGH && CurrentTimestamp - PreviousTimestamp >= LedOnDurationMSec)
  {
    digitalWrite(TRANSMIT_ID_CONTROL_PIN, LOW);
    LedEnablePinState = LOW;
    PreviousTimestamp = CurrentTimestamp;
  }
}

// Read Buggy ID
uint8_t ReadBuggyID()
{
  return (digitalRead(BUGGY_ID_PIN_3) << 3) |
         (digitalRead(BUGGY_ID_PIN_2) << 2) |
         (digitalRead(BUGGY_ID_PIN_1) << 1) |
         (digitalRead(BUGGY_ID_PIN_0));
}

// Set motor PWM
void SetMotorControlParameters(int16_t PWMValue, uint8_t HBridgeControlPinA, uint8_t HBridgeControlPinB)
{
  if (PWMValue >= 0)
  {
    analogWrite(HBridgeControlPinB, PWMValue);
    digitalWrite(HBridgeControlPinA, LOW);
  }
  else
  {
    analogWrite(HBridgeControlPinA, -PWMValue);
    digitalWrite(HBridgeControlPinB, LOW);
  }
}

void UpdateMotorSpeed(int16_t LeftMotorPWMValue, int16_t RightMotorPWMValue)
{
  SetMotorControlParameters(LeftMotorPWMValue, MOTOR_L1_PIN, MOTOR_L2_PIN);
  SetMotorControlParameters(RightMotorPWMValue, MOTOR_R1_PIN, MOTOR_R2_PIN);
}

// ---------------- LDR Tracking with Spike Duration ----------------

struct LDRSpikeTracker {
  bool inSpike = false;
  uint32_t spikeStartTime = 0;
};

LDRSpikeTracker leftTracker, rightTracker;

void TrackAndPrintLDR(uint16_t L, uint16_t R)
{
  uint32_t currentMillis = millis();

  // Left LDR
  if (L > LDR_SPIKE_THRESHOLD)
  {
    if (!leftTracker.inSpike)
    {
      leftTracker.inSpike = true;
      leftTracker.spikeStartTime = currentMillis;
    }
  }
  else
  {
    if (leftTracker.inSpike)
    {
      uint32_t spikeDuration = currentMillis - leftTracker.spikeStartTime;
      Serial.print("Left LDR spike duration: "); Serial.print(spikeDuration); Serial.println(" ms");
      leftTracker.inSpike = false;
    }
  }

  // Right LDR
  if (R > LDR_SPIKE_THRESHOLD)
  {
    if (!rightTracker.inSpike)
    {
      rightTracker.inSpike = true;
      rightTracker.spikeStartTime = currentMillis;
    }
  }
  else
  {
    if (rightTracker.inSpike)
    {
      uint32_t spikeDuration = currentMillis - rightTracker.spikeStartTime;
      Serial.print("Right LDR spike duration: "); Serial.print(spikeDuration); Serial.println(" ms");
      rightTracker.inSpike = false;
    }
  }
}

// Leader mode light tracking
void TrackLightLeaderMode()
{
  static uint32_t PreviousTimestamp = millis();
  if (millis() - PreviousTimestamp < LIGHT_TRACKING_LEADER_MODE_UPDATE_PERIOD_MS) return;

  uint16_t L = MeasureLDRCircuitVoltage(LDR_L_PIN);
  uint16_t R = MeasureLDRCircuitVoltage(LDR_R_PIN);
  int16_t LM, RM;

 Serial.print("LDR L: "); Serial.print(L);
  Serial.print("  |  LDR R: "); Serial.println(R);

  TrackAndPrintLDR(L, R);

  if (abs(L - R) > LDR_PAIR_TURN_THRESHOLD)
  {
    if (L > R) { LM = MOTOR_L_LEFT_TURN_PWM_VALUE; RM = MOTOR_R_LEFT_TURN_PWM_VALUE; }
    else       { LM = MOTOR_L_RIGHT_TURN_PWM_VALUE; RM = MOTOR_R_RIGHT_TURN_PWM_VALUE; }
  }
  else
  {
    if (min(L, R) > LDR_STOP_LEVEL_THRESHOLD) LM = RM = 0;
    else LM = MOTOR_L_FORWARD_PWM_VALUE, RM = MOTOR_R_FORWARD_PWM_VALUE;
  }

  UpdateMotorSpeed(LM, RM);
  PreviousTimestamp = millis();
}

// Follower mode light tracking
void TrackLightFollowerMode(uint8_t LockState)
{
  static uint32_t PreviousTimestamp = millis();
  if (millis() - PreviousTimestamp < LIGHT_TRACKING_FOLLOWER_MODE_UPDATE_PERIOD_MS) return;

  int16_t LM, RM;

  uint16_t L = MeasureLDRCircuitVoltage(LDR_L_PIN);
  uint16_t R = MeasureLDRCircuitVoltage(LDR_R_PIN);

 // Serial.print("LDR L: "); Serial.print(L);
 // Serial.print("  |  LDR R: "); Serial.println(R);

  TrackAndPrintLDR(L, R);

  if (LockState == 1)
  {
    if (abs(L - R) > LDR_PAIR_TURN_THRESHOLD)
    {
      if (L > R) { LM = MOTOR_L_LEFT_TURN_PWM_VALUE; RM = MOTOR_R_LEFT_TURN_PWM_VALUE; }
      else       { LM = MOTOR_L_RIGHT_TURN_PWM_VALUE; RM = MOTOR_R_RIGHT_TURN_PWM_VALUE; }
    }
    else
    {
      if (min(L, R) > LDR_STOP_LEVEL_THRESHOLD) LM = RM = 0;
      else LM = MOTOR_L_FORWARD_PWM_VALUE, RM = MOTOR_R_FORWARD_PWM_VALUE;
    }
  }
  else
  {
    LM = RM = 0; // SEEK pattern
  }

  UpdateMotorSpeed(LM, RM);
  PreviousTimestamp = millis();
}

// ---------------- Setup ----------------
void setup()
{
  Serial.begin(UART_BAUDRATE);
  pinMode(BUGGY_ID_PIN_3, INPUT_PULLUP);
  pinMode(BUGGY_ID_PIN_2, INPUT_PULLUP);
  pinMode(BUGGY_ID_PIN_1, INPUT_PULLUP);
  pinMode(BUGGY_ID_PIN_0, INPUT_PULLUP);
  pinMode(MOTOR_L1_PIN, OUTPUT);
  pinMode(MOTOR_L2_PIN, OUTPUT);
  pinMode(MOTOR_R1_PIN, OUTPUT);
  pinMode(MOTOR_R2_PIN, OUTPUT);
  pinMode(LOCK_INDICATOR_PIN, OUTPUT);
  pinMode(TRANSMIT_ID_CONTROL_PIN, OUTPUT);
  pinMode(SEEK_INDICATOR_PIN, OUTPUT);

  digitalWrite(MOTOR_L1_PIN, LOW);
  digitalWrite(MOTOR_L2_PIN, LOW);
  digitalWrite(MOTOR_R1_PIN, LOW);
  digitalWrite(MOTOR_R2_PIN, LOW);
  digitalWrite(LOCK_INDICATOR_PIN, LOW);
  digitalWrite(TRANSMIT_ID_CONTROL_PIN, LOW);
  digitalWrite(SEEK_INDICATOR_PIN, LOW);

  BuggyID = ReadBuggyID();

  Serial.println("B37VB Conga Line Example");
  Serial.print("Version: "); Serial.println(FIRMWARE_VERSION);
  Serial.print("Buggy ID: "); Serial.println(BuggyID);

  if (BuggyID >= CONGA_LINE_MAX_BUGGIES)
  {
    Serial.println("Buggy ID too large! Halting!");
    while (1) {}
  }
}

// ---------------- Loop ----------------
void loop()
{
  TransmitBuggyIDPulses();

  if (BuggyID != BUGGY_ID_LEADER_MODE)
  {
    uint8_t LockState = DetectBuggyIDPulses();

    // 🔴 SEEK LED: ON if not locked, OFF if locked
    digitalWrite(SEEK_INDICATOR_PIN, LockState == 0 ? HIGH : LOW);

    TrackLightFollowerMode(LockState);
  }
  else
  {
    TrackLightLeaderMode();
  }
}