#include <IRremote.h>

#define IR_PIN 11
#define MOTION_PIN 2
#define LDR_PIN A0
#define MODE_BUTTON 8
#define BUZZER 9
#define SYS_LED 10
#define GREEN_LED 12
#define YELLOW_LED 13

#define LED1 3  // Dining
#define LED2 4
#define LED3 5
#define LED4 6
#define LED5 7

IRrecv irrecv(IR_PIN);
decode_results results;

bool autoMode = false;
bool diningOffLast = false;
unsigned long lastMotion = 0;
unsigned long lastBlink = 0;
bool sysLedState = false;
bool buttonPrev = HIGH;
bool isNight = false;

void setup() {
  Serial.begin(9600);
  irrecv.enableIRIn();

  pinMode(MOTION_PIN, INPUT);
  pinMode(MODE_BUTTON, INPUT_PULLUP);
  pinMode(LDR_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(SYS_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED5, OUTPUT);

  // Read light level at startup
  int lightValue = analogRead(LDR_PIN);
  isNight = lightValue < 500; // adjust threshold
  autoMode = isNight;         // night -> auto, day -> manual
  updateModeLEDs();
}

void loop() {
  // Blink system LED every 500 ms
  if (millis() - lastBlink >= 500) {
    sysLedState = !sysLedState;
    digitalWrite(SYS_LED, sysLedState);
    lastBlink = millis();
  }

  // Re-evaluate day/night every 5 seconds
  static unsigned long lastLdrCheck = 0;
  if (millis() - lastLdrCheck > 5000) {
    int lightValue = analogRead(LDR_PIN);
    bool nightNow = lightValue < 500;
    if (nightNow != isNight) {
      isNight = nightNow;
      autoMode = isNight; // switch automatically
      updateModeLEDs();
    }
    lastLdrCheck = millis();
  }

  // Manual override button
  bool buttonState = digitalRead(MODE_BUTTON);
  if (buttonPrev == HIGH && buttonState == LOW) {
    autoMode = !autoMode;
    updateModeLEDs();
    delay(200);
  }
  buttonPrev = buttonState;

  // ---------------- AUTO MODE ----------------
  if (autoMode) {
    if (isNight && digitalRead(MOTION_PIN) == HIGH) {
      beep();
      if (!diningOffLast) {
        // First motion → all lights ON
        turnAllLEDs(HIGH);
        diningOffLast = true;
      } else {
        // Second motion → turn OFF dining room only
        digitalWrite(LED1, LOW);
        diningOffLast = false;
      }
      lastMotion = millis();
      delay(1000); // debounce
    }

    // Auto-off after 30 s without motion
    if (millis() - lastMotion > 30000) {
      turnAllLEDs(LOW);
    }
  }

  // ---------------- MANUAL MODE ----------------
  if (!autoMode && irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    switch (results.value) {
      case 0xFF30CF: toggleLED(LED1); break; // 1
      case 0xFF18E7: toggleLED(LED2); break; // 2
      case 0xFF7A85: toggleLED(LED3); break; // 3
      case 0xFF10EF: toggleLED(LED4); break; // 4
      case 0xFF38C7: toggleLED(LED5); break; // 5
    }
    irrecv.resume();
  }
}

// ---------------- HELPER FUNCTIONS ----------------
void updateModeLEDs() {
  digitalWrite(GREEN_LED, autoMode);
  digitalWrite(YELLOW_LED, !autoMode);
}

void toggleLED(int pin) {
  digitalWrite(pin, !digitalRead(pin));
}

void turnAllLEDs(bool state) {
  digitalWrite(LED1, state);
  digitalWrite(LED2, state);
  digitalWrite(LED3, state);
  digitalWrite(LED4, state);
  digitalWrite(LED5, state);
}

void beep() {
  digitalWrite(BUZZER, HIGH);
  delay(100);
  digitalWrite(BUZZER, LOW);
}
