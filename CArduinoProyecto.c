#include <Servo.h>

Servo servo1;
Servo servo2;

#define S0 4
#define S1 5
#define S2 6
#define S3 7
#define salidaTCS 8

// Si sin objeto delante los servos se mueven solos, baja este numero.
// Pon un valor un poco mayor que el R/V/A mas bajo que viste con un objeto delante.
#define UMBRAL_OBJETO 150

unsigned long leerColor(bool s2, bool s3) {
  digitalWrite(S2, s2);
  digitalWrite(S3, s3);
  delay(50);
  return pulseIn(salidaTCS, LOW, 100000UL);
}

void setup() {
  servo1.attach(9);
  servo2.attach(10);
  servo1.write(0);
  servo2.write(0);

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(salidaTCS, INPUT);

  digitalWrite(S0, HIGH);   // frecuencia al 20 %
  digitalWrite(S1, LOW);

  Serial.begin(9600);
}

void loop() {
  unsigned long rojo  = leerColor(LOW,  LOW);
  unsigned long verde = leerColor(HIGH, HIGH);
  unsigned long azul  = leerColor(LOW,  HIGH);

  Serial.print("R:");   Serial.print(rojo);
  Serial.print("\tV:"); Serial.print(verde);
  Serial.print("\tA:"); Serial.println(azul);

  if (rojo == 0 || verde == 0 || azul == 0) {
    return;   // lectura fallida, se ignora
  }

  if (rojo < verde && rojo < azul && rojo < UMBRAL_OBJETO) {
    Serial.println("ROJO");
    servo1.write(45);
    delay(2500);
    servo1.write(0);
    delay(500);
  }
  else if (verde < rojo && verde < azul && verde < UMBRAL_OBJETO) {
    Serial.println("VERDE");
    servo2.write(45);
    delay(2500);
    servo2.write(0);
    delay(500);
  }
  else if (azul < rojo && azul < verde && azul < UMBRAL_OBJETO) {
    Serial.println("AZUL");
  }

  delay(200);
}