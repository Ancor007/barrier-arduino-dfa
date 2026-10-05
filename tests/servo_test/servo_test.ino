/*
 * Тест сервопривода: плавный ход 0° <-> 90° с выводом угла в Serial Monitor.
 * Приложение 1 курсового проекта.
 */
#include <Servo.h>

#define SERVO_PIN 9

Servo testServo;

int angle = 0;
int direction = 1;  // 1 = вверх, -1 = вниз

unsigned long lastUpdate = 0;
const int stepDelay = 20;  // задержка между шагами

void setup() {
  Serial.begin(9600);
  testServo.attach(SERVO_PIN);
  Serial.println("[TEST] Servo test started");
}

void loop() {
  unsigned long now = millis();

  if (now - lastUpdate >= stepDelay) {
    lastUpdate = now;

    angle += direction;
    testServo.write(angle);

    // Вывод текущего угла каждые 10 градусов
    if (angle % 10 == 0) {
      Serial.print("[TEST] Angle: ");
      Serial.println(angle);
    }

    // Смена направления на границах
    if (angle >= 90) {
      direction = -1;
      Serial.println("[TEST] Reached top, going down");
    } else if (angle <= 0) {
      direction = 1;
      Serial.println("[TEST] Reached bottom, going up");
    }
  }
}
