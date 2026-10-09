
#include <Wire.h>
#include <U8g2lib.h>

// =====================================================
// OLED
// =====================================================
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(
  U8G2_R0,
  U8X8_PIN_NONE
);

#define OLED_SDA 21
#define OLED_SCL 22

// =====================================================
// L298N MOTOR DRIVER
// =====================================================
#define ENA_PIN 26
#define IN1_PIN 27
#define IN2_PIN 14

// =====================================================
// ULTRASONIC SENSOR
// =====================================================
#define TRIG_PIN 5
#define ECHO_PIN 18

// =====================================================
// BUTTONS
// =====================================================
#define BUTTON_UP     32
#define BUTTON_DOWN   33
#define BUTTON_SELECT 25
#define BUTTON_BACK   13

// =====================================================
// LED
// =====================================================
// Both LEDs are controlled by GPIO 23.
//
// GPIO 23 -> resistor -> LED 1 -> GND
// GPIO 23 -> resistor -> LED 2 -> GND
//
// IMPORTANT:
// Each LED must have its own resistor.
#define LED_PIN 23

// =====================================================
// BUZZER
// =====================================================
#define BUZZER_PIN 19

// =====================================================
// PUMP SETTINGS
// =====================================================

// Calibrated droplet rate.
//
// Based on your previous test:
//
// 20 mL = 146 droplets
//
// If the pump is approximately 50 mL/min:
//
// 50 mL/min x 7.3 droplets/mL
// = approximately 365 droplets/min
//
// IMPORTANT:
// Change this value after testing the actual pump.
float dropletsPerMinute = 365.0;

// 0   = OFF
// 255 = FULL SPEED
int pumpSpeed = 255;

// =====================================================
// PRESET DROPLETS
// =====================================================
//
// Based on:
//
// 20 mL = 146 droplets
//
// Approximate conversion:
//
// 25 mL = 183 droplets
// 50 mL = 365 droplets
//
const int droplets[] = {
  146,
  183,
  365
};

const int numberOfDropletPresets = 3;

// =====================================================
// LEVEL CHECK SETTINGS
// =====================================================
const unsigned long LEVEL_CHECK_TIME = 5000;
const unsigned long LEVEL_READ_INTERVAL = 100;

// =====================================================
// BUTTON STATES
// =====================================================
bool lastUpState = HIGH;
bool lastDownState = HIGH;
bool lastSelectState = HIGH;
bool lastBackState = HIGH;

// =====================================================
// SYSTEM STATES
// =====================================================
enum SystemState {

  MODE_MENU,

  DROPLET_MENU,

  PRESET_CONFIRM,

  PRESET_RUNNING,

  LEVEL_MENU,

  LEVEL_CHECKING,

  LEVEL_RESULT

};

SystemState state = MODE_MENU;

// =====================================================
// MENU VARIABLES
// =====================================================
int selectedMode = 0;
int selectedDroplet = 0;

// =====================================================
// LEVEL VARIABLES
// =====================================================
float finalLevelDistance = 0.0;

bool levelCheckSuccessful = false;

// =====================================================
// LED BLINK VARIABLES
// =====================================================
unsigned long lastBlinkTime = 0;

bool blinkState = false;

const unsigned long blinkInterval = 500;


// =====================================================
// READY LED
// =====================================================
void showReadyLED() {

  digitalWrite(
    LED_PIN,
    HIGH
  );
}


// =====================================================
// START DISPENSING LED MODE
// =====================================================
void startDispensingLED() {

  lastBlinkTime = millis();

  blinkState = false;

  digitalWrite(
    LED_PIN,
    LOW
  );
}


// =====================================================
// UPDATE DISPENSING LEDs
// =====================================================
void updateDispensingLEDs() {

  if (
    millis() - lastBlinkTime >=
    blinkInterval
  ) {

    lastBlinkTime = millis();

    blinkState = !blinkState;

    digitalWrite(
      LED_PIN,
      blinkState
    );
  }
}


// =====================================================
// BEEP BEEP BEEP
// =====================================================
// Plays three short beeps when dispensing is complete.
void drinkReadyBuzzer() {

  for (int i = 0; i < 3; i++) {

    tone(
      BUZZER_PIN,
      2000
    );

    delay(500);

    noTone(
      BUZZER_PIN
    );

    delay(500);
  }
}


// =====================================================
// PUMP ON
// =====================================================
//
// REVERSED PUMP DIRECTION:
//
// IN1 = LOW
// IN2 = HIGH
//
// ENA controls speed.
void pumpON() {

  digitalWrite(
    IN1_PIN,
    LOW
  );

  digitalWrite(
    IN2_PIN,
    HIGH
  );

  analogWrite(
    ENA_PIN,
    pumpSpeed
  );
}


// =====================================================
// PUMP OFF
// =====================================================
void pumpOFF() {

  analogWrite(
    ENA_PIN,
    0
  );

  digitalWrite(
    IN1_PIN,
    LOW
  );

  digitalWrite(
    IN2_PIN,
    LOW
  );
}


// =====================================================
// BUTTON PRESS DETECTION
// =====================================================
bool buttonPressed(
  int pin,
  bool &lastState
) {

  bool currentState =
    digitalRead(pin);

  if (
    lastState == HIGH &&
    currentState == LOW
  ) {

    lastState = LOW;

    delay(300);

    return true;
  }

  lastState =
    currentState;

  return false;
}


// =====================================================
// ULTRASONIC DISTANCE
// =====================================================
float getDistanceCM() {

  digitalWrite(
    TRIG_PIN,
    LOW
  );

  delayMicroseconds(2);

  digitalWrite(
    TRIG_PIN,
    HIGH
  );

  delayMicroseconds(10);

  digitalWrite(
    TRIG_PIN,
    LOW
  );

  long duration =
    pulseIn(
      ECHO_PIN,
      HIGH,
      30000
    );

  if (
    duration == 0
  ) {

    return -1;
  }

  float distance =
    duration * 0.0343 / 2.0;

  if (
    distance < 2.0 ||
    distance > 400.0
  ) {

    return -1;
  }

  return distance;
}


// =====================================================
// MAIN MODE MENU
// =====================================================
void showModeMenu() {

  oled.clearBuffer();

  oled.setFont(
    u8g2_font_6x10_tf
  );

  oled.drawStr(
    0,
    10,
    "AUTOMATIC DISPENSER"
  );

  oled.drawStr(
    0,
    27,
    "SELECT MODE:"
  );

  if (
    selectedMode == 0
  ) {

    oled.drawStr(
      0,
      43,
      "> DROPLET MODE"
    );

    oled.drawStr(
      0,
      57,
      "  LEVEL SENSING"
    );

  } else {

    oled.drawStr(
      0,
      43,
      "  DROPLET MODE"
    );

    oled.drawStr(
      0,
      57,
      "> LEVEL SENSING"
    );
  }

  oled.sendBuffer();
}


// =====================================================
// DROPLET MENU
// =====================================================
void showDropletMenu() {

  oled.clearBuffer();

  oled.setFont(
    u8g2_font_6x10_tf
  );

  oled.drawStr(
    0,
    10,
    "SELECT DROPLETS"
  );

  for (
    int i = 0;
    i < numberOfDropletPresets;
    i++
  ) {

    int y =
      27 + (i * 12);

    if (
      i == selectedDroplet
    ) {

      oled.drawStr(
        0,
        y,
        ">"
      );

    } else {

      oled.drawStr(
        0,
        y,
        " "
      );
    }

    char buffer[20];

    sprintf(
      buffer,
      "%d DROPS",
      droplets[i]
    );

    oled.drawStr(
      15,
      y,
      buffer
    );
  }

  oled.sendBuffer();
}


// =====================================================
// PRESET CONFIRMATION
// =====================================================
void showPresetConfirm() {

  oled.clearBuffer();

  oled.setFont(
    u8g2_font_6x10_tf
  );

  oled.drawStr(
    0,
    12,
    "DROPLET PRESET"
  );

  char buffer[30];

  sprintf(
    buffer,
    "TARGET: %d DROPS",
    droplets[selectedDroplet]
  );

  oled.drawStr(
    0,
    30,
    buffer
  );

  oled.drawStr(
    0,
    45,
    "SELECT = START"
  );

  oled.drawStr(
    0,
    58,
    "BACK = CANCEL"
  );

  oled.sendBuffer();
}


// =====================================================
// PRESET RUNNING SCREEN
// =====================================================
void showRunning(
  float currentDrops,
  float targetDrops
) {

  oled.clearBuffer();

  oled.setFont(
    u8g2_font_6x10_tf
  );

  oled.drawStr(
    0,
    10,
    "DISPENSING..."
  );

  char buffer[30];

  sprintf(
    buffer,
    "TARGET: %.0f DROPS",
    targetDrops
  );

  oled.drawStr(
    0,
    28,
    buffer
  );

  sprintf(
    buffer,
    "DONE: %.0f DROPS",
    currentDrops
  );

  oled.drawStr(
    0,
    42,
    buffer
  );

  oled.drawStr(
    0,
    58,
    "BACK = STOP"
  );

  oled.sendBuffer();
}


// =====================================================
// LEVEL MENU
// =====================================================
void showLevelMenu() {

  oled.clearBuffer();

  oled.setFont(
    u8g2_font_6x10_tf
  );

  oled.drawStr(
    0,
    10,
    "LEVEL SENSING"
  );

  oled.drawStr(
    0,
    27,
    "CHECK WATER LEVEL"
  );

  oled.drawStr(
    0,
    43,
    "SELECT = CHECK"
  );

  oled.drawStr(
    0,
    58,
    "BACK = RETURN"
  );

  oled.sendBuffer();
}


// =====================================================
// LEVEL CHECKING SCREEN
// =====================================================
void showLevelChecking(
  unsigned long elapsed
) {

  oled.clearBuffer();

  oled.setFont(
    u8g2_font_6x10_tf
  );

  oled.drawStr(
    0,
    12,
    "CHECKING LEVEL..."
  );

  unsigned long seconds =
    elapsed / 1000;

  if (
    seconds > 5
  ) {

    seconds = 5;
  }

  char buffer[30];

  sprintf(
    buffer,
    "TIME: %lu / 5 sec",
    seconds
  );

  oled.drawStr(
    0,
    30,
    buffer
  );

  oled.drawStr(
    0,
    46,
    "PLEASE WAIT..."
  );

  oled.drawStr(
    0,
    60,
    "BACK = CANCEL"
  );

  oled.sendBuffer();
}


// =====================================================
// LEVEL RESULT SCREEN
// =====================================================
void showLevelResult() {

  oled.clearBuffer();

  oled.setFont(
    u8g2_font_6x10_tf
  );

  if (
    levelCheckSuccessful
  ) {

    oled.drawStr(
      0,
      10,
      "WATER LEVEL"
    );

    char buffer[30];

    sprintf(
      buffer,
      "DISTANCE: %.1f cm",
      finalLevelDistance
    );

    oled.drawStr(
      0,
      28,
      buffer
    );

    oled.drawStr(
      0,
      44,
      "WATER DETECTED"
    );

    oled.drawStr(
      0,
      59,
      "SELECT/BACK"
    );

  } else {

    oled.drawStr(
      0,
      10,
      "LEVEL SENSOR"
    );

    oled.drawStr(
      0,
      28,
      "ERROR!"
    );

    oled.drawStr(
      0,
      43,
      "CHECK WIRING"
    );

    oled.drawStr(
      0,
      59,
      "SELECT/BACK"
    );
  }

  oled.sendBuffer();
}


// =====================================================
// CHECK WATER LEVEL FOR 5 SECONDS
// =====================================================
void checkWaterLevel() {

  unsigned long startTime =
    millis();

  unsigned long lastReadingTime =
    0;

  float distanceSum = 0;

  int validReadings = 0;

  levelCheckSuccessful = false;

  finalLevelDistance = 0;

  // Make sure pump is OFF
  pumpOFF();

  // Ready LEDs ON
  showReadyLED();

  while (
    millis() - startTime <
    LEVEL_CHECK_TIME
  ) {

    // BACK = CANCEL
    if (
      digitalRead(BUTTON_BACK) == LOW
    ) {

      delay(300);

      lastBackState = HIGH;

      state = MODE_MENU;

      showModeMenu();

      return;
    }

    // Take reading every 100 ms
    if (
      millis() - lastReadingTime >=
      LEVEL_READ_INTERVAL
    ) {

      lastReadingTime =
        millis();

      float distance =
        getDistanceCM();

      // Valid reading
      if (
        distance > 0
      ) {

        distanceSum +=
          distance;

        validReadings++;
      }

      showLevelChecking(
        millis() - startTime
      );
    }
  }

  // Calculate final level
  if (
    validReadings > 0
  ) {

    finalLevelDistance =
      distanceSum /
      validReadings;

    levelCheckSuccessful =
      true;

  } else {

    levelCheckSuccessful =
      false;
  }

  state =
    LEVEL_RESULT;

  showLevelResult();
}


// =====================================================
// DROPLET-BASED DISPENSING
// =====================================================
void runPresetDispensing() {

  float targetDrops =
    droplets[selectedDroplet];

  // ===================================================
  // CALCULATE PUMP RUN TIME
  // ===================================================
  //
  // Example:
  //
  // 365 droplets/minute
  //
  // 146 drops = 24 seconds
  // 183 drops = 30 seconds
  // 365 drops = 60 seconds
  //
  // 146 / 24 = 6.08
  // 183 / 30 = 6.08
  // 365 / 60 = 6.08
  //
  // These values depend on actual calibration.
  //
  float timeMinutes =
    targetDrops /
    dropletsPerMinute;

  unsigned long totalTime =
    timeMinutes *
    60.0 *
    1000.0;

  unsigned long startTime =
    millis();

  // ===================================================
  // START LED BLINKING
  // ===================================================
  startDispensingLED();

  // ===================================================
  // START PUMP
  // ===================================================
  pumpON();

  // ===================================================
  // DISPENSING LOOP
  // ===================================================
  while (
    millis() - startTime <
    totalTime
  ) {

    // Both LEDs blink together
    updateDispensingLEDs();

    // =================================================
    // BACK = IMMEDIATE STOP
    // =================================================
    if (
      digitalRead(BUTTON_BACK) == LOW
    ) {

      // Stop pump
      pumpOFF();

      // LEDs return to ready
      showReadyLED();

      // Return to main menu
      state =
        MODE_MENU;

      delay(300);

      lastBackState =
        HIGH;

      showModeMenu();

      return;
    }

    // =================================================
    // CALCULATE DROPLET PROGRESS
    // =================================================
    unsigned long elapsed =
      millis() - startTime;

    float progress =
      (float)elapsed /
      (float)totalTime;

    float currentDrops =
      targetDrops *
      progress;

    if (
      currentDrops >
      targetDrops
    ) {

      currentDrops =
        targetDrops;
    }

    showRunning(
      currentDrops,
      targetDrops
    );

    delay(100);
  }

  // ===================================================
  // DISPENSING FINISHED
  // ===================================================

  // Stop pump
  pumpOFF();

  // LEDs stay ON
  showReadyLED();

  // Play Beep Beep Beep
  drinkReadyBuzzer();

  // ===================================================
  // RETURN TO MAIN MENU
  // ===================================================
  state =
    MODE_MENU;

  showModeMenu();
}


// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(115200);

  // ===================================================
  // I2C
  // ===================================================
  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );

  // ===================================================
  // OLED
  // ===================================================
  oled.begin();

  // ===================================================
  // L298N
  // ===================================================
  pinMode(
    ENA_PIN,
    OUTPUT
  );

  pinMode(
    IN1_PIN,
    OUTPUT
  );

  pinMode(
    IN2_PIN,
    OUTPUT
  );

  pumpOFF();

  // ===================================================
  // ULTRASONIC
  // ===================================================
  pinMode(
    TRIG_PIN,
    OUTPUT
  );

  pinMode(
    ECHO_PIN,
    INPUT
  );

  // ===================================================
  // BUTTONS
  // ===================================================
  pinMode(
    BUTTON_UP,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_DOWN,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_SELECT,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_BACK,
    INPUT_PULLUP
  );

  // ===================================================
  // LED
  // ===================================================
  pinMode(
    LED_PIN,
    OUTPUT
  );

  // ===================================================
  // BUZZER
  // ===================================================
  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  noTone(
    BUZZER_PIN
  );

  // ===================================================
  // SYSTEM READY
  // ===================================================
  showReadyLED();

  showModeMenu();
}


// =====================================================
// MAIN LOOP
// =====================================================
void loop() {

  // ===================================================
  // MODE MENU
  // ===================================================
  if (
    state == MODE_MENU
  ) {

    // UP
    if (
      buttonPressed(
        BUTTON_UP,
        lastUpState
      )
    ) {

      selectedMode--;

      if (
        selectedMode < 0
      ) {

        selectedMode = 1;
      }

      showModeMenu();
    }

    // DOWN
    if (
      buttonPressed(
        BUTTON_DOWN,
        lastDownState
      )
    ) {

      selectedMode++;

      if (
        selectedMode > 1
      ) {

        selectedMode = 0;
      }

      showModeMenu();
    }

    // SELECT
    if (
      buttonPressed(
        BUTTON_SELECT,
        lastSelectState
      )
    ) {

      if (
        selectedMode == 0
      ) {

        selectedDroplet = 0;

        state =
          DROPLET_MENU;

        showDropletMenu();

      } else {

        state =
          LEVEL_MENU;

        showLevelMenu();
      }
    }
  }


  // ===================================================
  // DROPLET MENU
  // ===================================================
  else if (
    state == DROPLET_MENU
  ) {

    // UP
    if (
      buttonPressed(
        BUTTON_UP,
        lastUpState
      )
    ) {

      selectedDroplet--;

      if (
        selectedDroplet < 0
      ) {

        selectedDroplet =
          numberOfDropletPresets - 1;
      }

      showDropletMenu();
    }

    // DOWN
    if (
      buttonPressed(
        BUTTON_DOWN,
        lastDownState
      )
    ) {

      selectedDroplet++;

      if (
        selectedDroplet >=
        numberOfDropletPresets
      ) {

        selectedDroplet = 0;
      }

      showDropletMenu();
    }

    // SELECT
    if (
      buttonPressed(
        BUTTON_SELECT,
        lastSelectState
      )
    ) {

      state =
        PRESET_CONFIRM;

      showPresetConfirm();
    }

    // BACK
    if (
      buttonPressed(
        BUTTON_BACK,
        lastBackState
      )
    ) {

      state =
        MODE_MENU;

      showModeMenu();
    }
  }


  // ===================================================
  // PRESET CONFIRM
  // ===================================================
  else if (
    state == PRESET_CONFIRM
  ) {

    // SELECT = START
    if (
      buttonPressed(
        BUTTON_SELECT,
        lastSelectState
      )
    ) {

      state =
        PRESET_RUNNING;

      runPresetDispensing();
    }

    // BACK = CANCEL
    if (
      buttonPressed(
        BUTTON_BACK,
        lastBackState
      )
    ) {

      state =
        DROPLET_MENU;

      showDropletMenu();
    }
  }


  // ===================================================
  // LEVEL MENU
  // ===================================================
  else if (
    state == LEVEL_MENU
  ) {

    // SELECT
    if (
      buttonPressed(
        BUTTON_SELECT,
        lastSelectState
      )
    ) {

      state =
        LEVEL_CHECKING;

      checkWaterLevel();
    }

    // BACK
    if (
      buttonPressed(
        BUTTON_BACK,
        lastBackState
      )
    ) {

      state =
        MODE_MENU;

      showModeMenu();
    }
  }


  // ===================================================
  // LEVEL RESULT
  // ===================================================
  else if (
    state == LEVEL_RESULT
  ) {

    // SELECT = CHECK AGAIN
    if (
      buttonPressed(
        BUTTON_SELECT,
        lastSelectState
      )
    ) {

      state =
        LEVEL_CHECKING;

      checkWaterLevel();
    }

    // BACK
    if (
      buttonPressed(
        BUTTON_BACK,
        lastBackState
      )
    ) {

      state =
        MODE_MENU;

      showModeMenu();
    }
  }
}