/*
 * Транспортный шлагбаум на Arduino Uno: управление через ДКА.
 * Курсовой проект по дисциплине «Микропроцессорные системы», СамГТУ, 2025.
 *
 * Библиотеки: Servo, IRremote (v4.x)
 * Подробности: README.md и docs/
 */
#include <Servo.h>
#include <IRremote.h>

#define SERVO_PIN 9
#define LDR_PIN A3
#define IR_PIN 2
#define STATUS_LED 13

#define SERVO_OPEN 90
#define SERVO_CLOSE 0

#define AUTO_CLOSE_DELAY 5000
#define LED_BLINK_INTERVAL 150

// гистерезис освещенности
#define LIGHT_CAN_CLOSE 150
#define LIGHT_CANNOT_CLOSE 200

// скорости
#define SERVO_STEP_DELAY_NORMAL 20   // обычная скорость
#define SERVO_STEP_DELAY_FAST   5    // аварийная (быстро вверх)

Servo gateServo;

bool gateOpen = false;
bool moving = false;
bool canClose = false;

int currentAngle = SERVO_CLOSE;
int targetAngle  = SERVO_CLOSE;

// направление
bool closingNow = false;

unsigned long lastActionTime = 0;
unsigned long ledTimer = 0;
unsigned long servoStepTimer = 0;
unsigned long currentStepDelay = SERVO_STEP_DELAY_NORMAL;

bool ledState = false;

// ===== запуск движения =====
void moveGate(int angle, bool isClosing) {
  gateServo.attach(SERVO_PIN);
  targetAngle = angle;
  moving = true;
  closingNow = isClosing;
  currentStepDelay = SERVO_STEP_DELAY_NORMAL;
  servoStepTimer = millis();

  Serial.print("[MOVE] Start move to ");
  Serial.print(angle);
  Serial.println(isClosing ? " (closing)" : " (opening)");
}

// ===== setup =====
void setup() {
  Serial.begin(9600);
  pinMode(STATUS_LED, OUTPUT);

  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  gateServo.attach(SERVO_PIN);
  gateServo.write(SERVO_CLOSE);
  currentAngle = SERVO_CLOSE;
  gateServo.detach();

  Serial.println("[SETUP] Ready");
}

// ===== loop =====
void loop() {
  unsigned long now = millis();

  // ===== датчик освещенности =====
  int sensorValue = analogRead(LDR_PIN);

  if (sensorValue <= LIGHT_CAN_CLOSE) {
    canClose = true;
  }
  else if (sensorValue >= LIGHT_CANNOT_CLOSE) {
    canClose = false;
  }

  // ===== АВАРИЙНОЕ ПРЕРЫВАНИЕ =====
  if (moving && closingNow && !canClose) {
    Serial.println("[EMERGENCY] Obstacle detected! FAST OPEN");
    targetAngle = SERVO_OPEN;
    closingNow = false;
    gateOpen = true;
    currentStepDelay = SERVO_STEP_DELAY_FAST; // максимальная скорость
  }

  // ===== плавное движение =====
  if (moving) {
    if (now - servoStepTimer >= currentStepDelay) {
      servoStepTimer = now;

      if (currentAngle < targetAngle) {
        currentAngle++;
        gateServo.write(currentAngle);
      }
      else if (currentAngle > targetAngle) {
        currentAngle--;
        gateServo.write(currentAngle);
      }
      else {
        moving = false;
        gateServo.detach();
        Serial.println("[MOVE] Movement finished");
      }
    }
  }

  // ===== мигание LED при движении =====
  if (moving) {
    if (now - ledTimer >= LED_BLINK_INTERVAL) {
      ledTimer = now;
      ledState = !ledState;
      digitalWrite(STATUS_LED, ledState);
    }
  } else {
    digitalWrite(STATUS_LED, LOW);
  }

  // ===== ИК-пульт =====
  if (IrReceiver.decode()) {
    Serial.println("[IR] Trigger");

    if (!gateOpen && !moving) {
      moveGate(SERVO_OPEN, false);
      gateOpen = true;
      lastActionTime = now;
    }

    IrReceiver.resume();
  }

  // ===== автозакрытие =====
  if (gateOpen && !moving && now - lastActionTime >= AUTO_CLOSE_DELAY) {
    if (canClose) {
      Serial.println("[AUTO] Closing");
      moveGate(SERVO_CLOSE, true);
      gateOpen = false;
    } else {
      lastActionTime = now;
      Serial.println("[AUTO] Close blocked");
    }
  }
}
