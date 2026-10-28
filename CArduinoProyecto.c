#include <Servo.h>


const int pinIR = 2; 

const int s0 = 3, s1 = 4, s2 = 5, s3 = 6, out = 7; 

const int motorAdelante = 8;
const int motorPWM = 9; 

const int pinServoIzq = 10;
const int pinServoDer = 11;

Servo servoIzquierdo;
Servo servoDerecho;

void setup() {
  Serial.begin(115200); 
  
  pinMode(pinIR, INPUT);
  pinMode(s0, OUTPUT); pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT); pinMode(s3, OUTPUT);
  pinMode(out, INPUT);
  
  pinMode(motorAdelante, OUTPUT);
  pinMode(motorPWM, OUTPUT);
  
  servoIzquierdo.attach(pinServoIzq);
  servoDerecho.attach(pinServoDer);
  
  
  digitalWrite(s0, HIGH);
  digitalWrite(s1, LOW);
  
  resetServos();
  encenderBanda();
  Serial.println("ESTADO: EN MOVIMIENTO");
}

void loop() {
  
  if (digitalRead(pinIR) == LOW) { 
    apagarBanda();
    Serial.println("ESTADO: DETENIDA");
    delay(500); 
    
    String color = leerRGB();
    Serial.print("COLOR:");
    Serial.println(color); 
    
    desviarCaja(color);
    
    delay(1000); 
    resetServos();
    
    encenderBanda();
    Serial.println("ESTADO: EN MOVIMIENTO");
    delay(1000); 
  }
}

void encenderBanda() {
  digitalWrite(motorAdelante, HIGH);
  analogWrite(motorPWM, 150); 
}

void apagarBanda() {
  digitalWrite(motorAdelante, LOW);
  analogWrite(motorPWM, 0);
}

void resetServos() {
  servoIzquierdo.write(90); 
  servoDerecho.write(90);
}

void desviarCaja(String color) {
  if (color == "ROJO") {
    servoIzquierdo.write(45); 
  } else if (color == "AZUL") {
    servoDerecho.write(135); 
  } else if (color == "AMARILLO") {
    
  }
}

String leerRGB() {
 
  digitalWrite(s2, LOW); digitalWrite(s3, LOW);
  int rojo = pulseIn(out, LOW);
  
  
  digitalWrite(s2, LOW); digitalWrite(s3, HIGH);
  int azul = pulseIn(out, LOW);
  
  
  if (rojo < azul && rojo < 100) return "ROJO";
  else if (azul < rojo && azul < 100) return "AZUL";
  else return "AMARILLO"; 
}