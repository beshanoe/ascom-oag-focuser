constexpr auto DEVICE_GUID = "9ee5c8d6-aa7c-43c8-bd70-2e8c540a3ea6";

constexpr auto OK = "OK";
constexpr auto NOK = "NOK";

constexpr auto TRUE = "TRUE";
constexpr auto FALSE = "FALSE";

constexpr auto COMMAND_PING = "COMMAND:PING";
constexpr auto RESULT_PING = "RESULT:PING:OK:";

constexpr auto COMMAND_INFO = "COMMAND:INFO";
constexpr auto RESULT_INFO = "RESULT:DarkSkyGeek's Telescope Focuser Firmware v1.0";

constexpr auto COMMAND_CONFIG_GETMS1 = "COMMAND:CONFIG:GETMS1";
constexpr auto COMMAND_CONFIG_GETMS2 = "COMMAND:CONFIG:GETMS2";
constexpr auto COMMAND_CONFIG_GETDISABLE_WHEN_IDLE = "COMMAND:CONFIG:GETDISABLE_WHEN_IDLE";
constexpr auto COMMAND_CONFIG_GET_STEP_INTERVAL = "COMMAND:CONFIG:GET_STEP_INTERVAL";

constexpr auto COMMAND_CONFIG_SETMS1 = "COMMAND:CONFIG:SETMS1:";
constexpr auto COMMAND_CONFIG_SETMS2 = "COMMAND:CONFIG:SETMS2:";
constexpr auto COMMAND_CONFIG_DISABLE_WHEN_IDLE = "COMMAND:CONFIG:DISABLE_WHEN_IDLE:";
constexpr auto COMMAND_CONFIG_SET_STEP_INTERVAL = "COMMAND:CONFIG:SET_STEP_INTERVAL:";

constexpr auto COMMAND_FOCUSER_GETPOSITION = "COMMAND:FOCUSER:GETPOSITION";
constexpr auto COMMAND_FOCUSER_ISMOVING = "COMMAND:FOCUSER:ISMOVING";
constexpr auto COMMAND_FOCUSER_SETZEROPOSITION = "COMMAND:FOCUSER:SETZEROPOSITION";
constexpr auto COMMAND_FOCUSER_MOVE = "COMMAND:FOCUSER:MOVE:";
constexpr auto COMMAND_FOCUSER_HALT = "COMMAND:FOCUSER:HALT";

constexpr auto ERROR_INVALID_COMMAND = "ERROR:INVALID_COMMAND";

#define STEP_PIN 1
#define DIR_PIN 0
#define ENABLED_PIN 4
#define MS1_PIN 3
#define MS2_PIN 2
#define DEFAULT_POSITION 1000

volatile long targetPosition = 0;
volatile long currentPosition = 0;
unsigned long previousStepTime = 0;
long stepIntervalMicros = 5000;  // Adjust as per the stepper's speed rating.
bool shouldDisableWhenIdle = false;

void setup() {
  // Initialize serial port I/O.
  Serial.begin(115200);
  while (!Serial) {
    ;  // Wait for serial port to connect. Required for native USB!
  }
  Serial.flush();

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLED_PIN, OUTPUT);
  pinMode(MS1_PIN, OUTPUT);
  pinMode(MS2_PIN, OUTPUT);
  digitalWrite(ENABLED_PIN, shouldDisableWhenIdle ? HIGH : LOW);
  digitalWrite(MS1_PIN, LOW);
  digitalWrite(MS2_PIN, LOW);

  // hack until we can get EEPROM working
  currentPosition = DEFAULT_POSITION;
  targetPosition = currentPosition;
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    if (command == COMMAND_PING) {
      handlePing();
    } else if (command == COMMAND_INFO) {
      sendFirmwareInfo();
    } else if (command == COMMAND_FOCUSER_ISMOVING) {
      sendFocuserState();
    } else if (command == COMMAND_FOCUSER_GETPOSITION) {
      sendFocuserPosition();
    } else if (command == COMMAND_FOCUSER_SETZEROPOSITION) {
      setFocuserZeroPosition();
    } else if (command.startsWith(COMMAND_FOCUSER_MOVE)) {
      String arg = command.substring(strlen(COMMAND_FOCUSER_MOVE));
      int value = arg.toInt();
      serialPrintResult(command);
      moveFocuser(value);
    } else if (command == COMMAND_FOCUSER_HALT) {
      haltFocuser();
    }
    // Configuration commands
    else if (command == COMMAND_CONFIG_GETMS1) {
      serialPrintResult(COMMAND_CONFIG_GETMS1);
      Serial.println(digitalRead(MS1_PIN) == HIGH ? TRUE : FALSE);
    } else if (command == COMMAND_CONFIG_GETMS2) {
      serialPrintResult(COMMAND_CONFIG_GETMS2);
      Serial.println(digitalRead(MS2_PIN) == HIGH ? TRUE : FALSE);
    } else if (command == COMMAND_CONFIG_GETDISABLE_WHEN_IDLE) {
      serialPrintResult(COMMAND_CONFIG_GETDISABLE_WHEN_IDLE);
      Serial.println(shouldDisableWhenIdle ? TRUE : FALSE);
    } else if (command == COMMAND_CONFIG_GET_STEP_INTERVAL) {
      serialPrintResult(COMMAND_CONFIG_GET_STEP_INTERVAL);
      Serial.println(stepIntervalMicros);
    } else if (command.startsWith(COMMAND_CONFIG_SETMS1)) {
      // arg as TRUE or FALSE
      String arg = command.substring(strlen(COMMAND_CONFIG_SETMS1));
      digitalWrite(MS1_PIN, arg == TRUE ? HIGH : LOW);
      serialPrintResult(command);
      Serial.println(OK);
    } else if (command.startsWith(COMMAND_CONFIG_SETMS2)) {
      // arg as TRUE or FALSE
      String arg = command.substring(strlen(COMMAND_CONFIG_SETMS2));
      digitalWrite(MS2_PIN, arg == TRUE ? HIGH : LOW);
      serialPrintResult(command);
      Serial.println(OK);
    } else if (command.startsWith(COMMAND_CONFIG_DISABLE_WHEN_IDLE)) {
      // arg as TRUE or FALSE
      String arg = command.substring(strlen(COMMAND_CONFIG_DISABLE_WHEN_IDLE));
      shouldDisableWhenIdle = arg == TRUE;
      if (!isMoving()) {
        digitalWrite(ENABLED_PIN, shouldDisableWhenIdle ? HIGH : LOW);
      }
      serialPrintResult(command);
      Serial.println(OK);
    } else if (command.startsWith(COMMAND_CONFIG_SET_STEP_INTERVAL)) {
      // arg as microseconds
      String arg = command.substring(strlen(COMMAND_CONFIG_SET_STEP_INTERVAL));
      stepIntervalMicros = arg.toInt();
      serialPrintResult(command);
      Serial.println(OK);
    } else {
      handleInvalidCommand();
    }
  }

  step();

  if (!Serial) {
    stop();
  }
}

void step() {
  if (targetPosition == currentPosition) {
    stop();
    return;
  }

  unsigned long currentMicros = micros();
  if (currentMicros - previousStepTime >= stepIntervalMicros) {
    if (targetPosition > currentPosition) {
      digitalWrite(DIR_PIN, LOW);
      currentPosition++;
    } else {
      digitalWrite(DIR_PIN, HIGH);
      currentPosition--;
    }

    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(1);  // A very short pulse.
    digitalWrite(STEP_PIN, LOW);

    previousStepTime = currentMicros;
  }
}

void stop() {
  // Make sure we don't take another step.
  targetPosition = currentPosition;

  // Store the final position in EEPROM.
  // EEPROM.put(EEPROM_POSITION_BASE_ADDR, position);

  // And de-energize the stepper, prevent heat build up, and eliminate vibrations.
  if (shouldDisableWhenIdle) {
    digitalWrite(ENABLED_PIN, HIGH);
  }
}

void serialPrintResult(String command) {
  Serial.print("RESULT:");
  Serial.print(command);
  // check if command ends with a colon
  if (command[command.length() - 1] != ':') {
    Serial.print(":");
  }
}

//-- FOCUSER HANDLING ------------------------------------------------------

bool isMoving() {
  return abs(targetPosition - currentPosition) > 0;
}

void sendFocuserState() {
  serialPrintResult(COMMAND_FOCUSER_ISMOVING);
  Serial.println(isMoving() ? TRUE : FALSE);
}

void sendFocuserPosition() {
  serialPrintResult(COMMAND_FOCUSER_GETPOSITION);
  Serial.println(currentPosition);
}

void setFocuserZeroPosition() {
  serialPrintResult(COMMAND_FOCUSER_SETZEROPOSITION);
  if (!isMoving()) {
    currentPosition = DEFAULT_POSITION;
    // EEPROM.put(EEPROM_POSITION_BASE_ADDR, position);
    Serial.println(OK);
  } else {
    // Cannot set zero position while focuser is still moving...
    Serial.println(NOK);
  }
}

void moveFocuser(int target_position) {
  if (isMoving()) {
    // Cannot move while focuser is still moving from previous request...
    Serial.println(NOK);
    return;
  }

  if (target_position >= 0) {
    // Enable the stepper.
    digitalWrite(ENABLED_PIN, LOW);
    targetPosition = target_position;
    Serial.println(OK);
  } else {
    // Cannot move to a negative position...
    Serial.println(NOK);
  }
}

void haltFocuser() {
  stop();
  serialPrintResult(COMMAND_FOCUSER_HALT);
  Serial.println(OK);
}

//-- MISCELLANEOUS ------------------------------------------------------------

void handlePing() {
  serialPrintResult(COMMAND_PING);
  Serial.println(DEVICE_GUID);
}

void sendFirmwareInfo() {
  serialPrintResult(COMMAND_INFO);
  Serial.println(RESULT_INFO);
}

void handleInvalidCommand() {
  Serial.println(ERROR_INVALID_COMMAND);
}