#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Servo.h>       // Add this line at the top
Servo myServo;           // Create servo object

#define enA 10 // Enable1 L298 Pin enA
#define in1 9  // Motor1 L298 Pin in1
#define in2 8  // Motor1 L298 Pin in2
#define in3 7  // Motor2 L298 Pin in3
#define in4 6  // Motor2 L298 Pin in4
#define enB 5  // Enable2 L298 Pin enB

#define ir_R 2  // Right flame sensor pin (digital)
#define ir_F 3  // Front flame sensor pin (digital)
#define ir_L 4  // Left flame sensor pin (digital)
#define servo A4 // Servo motor control pin
#define pump A5  // Water pump control pin

SoftwareSerial gpsSerial(4, 5);  // RX, TX for GPS
SoftwareSerial gsmSerial(11, 12);  // RX, TX for GSM
TinyGPSPlus gps;

const char phoneNumber[] = "+1234567890"; // Change to target phone number

int Speed = 160; // Motor speed (0-255)
bool s1, s2, s3;
char command;
unsigned long lastCmdTime = 0;
unsigned long cmdTimeout = 5000;
unsigned long pumpStartTime = 0;
unsigned long pumpTimeout = 10000;

void setup() {
    Serial.begin(9600);
    gpsSerial.begin(9600);
    gsmSerial.begin(9600);
    
    pinMode(ir_R, INPUT);
    pinMode(ir_F, INPUT);
    pinMode(ir_L, INPUT);
    pinMode(pump, OUTPUT);
    
    pinMode(enA, OUTPUT);
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    pinMode(in3, OUTPUT);
    pinMode(in4, OUTPUT);
    pinMode(enB, OUTPUT);
    pinMode(servo, OUTPUT);
    
    digitalWrite(pump, LOW);
    analogWrite(enA, Speed);
    analogWrite(enB, Speed);
}

void loop() {
    if (Serial.available() > 0) {
        command = Serial.read();
        handleBluetoothCommand(command);
        lastCmdTime = millis();
    }
    if (millis() - lastCmdTime > cmdTimeout) {
        autonomousMode();
    }
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }
    
    bool flameDetected = (digitalRead(ir_R) == LOW || digitalRead(ir_F) == LOW || digitalRead(ir_L) == LOW);
   
    if (flameDetected) {
    Stop();
    digitalWrite(pump, HIGH);
    sendLocationSMS();

    // Sweep servo for 10 seconds while spraying
    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
        sweepServoWhilePumping();
    }

    digitalWrite(pump, LOW);
    myServo.write(90); // Reset to center
}
    delay(10);
}
    void sweepServoWhilePumping() {
    // Sweep from 60° to 120° and back while pump is ON
    for (int angle = 60; angle <= 120; angle += 5) {
        myServo.write(angle);
        delay(100);
    }
    for (int angle = 120; angle >= 60; angle -= 5) {
        myServo.write(angle);
        delay(100);
    }
}
void autonomousMode() {
    s1 = digitalRead(ir_R) == LOW;
    s2 = digitalRead(ir_F) == LOW;
    s3 = digitalRead(ir_L) == LOW;

    if (s1 || s2 || s3) {
        Stop();
        digitalWrite(pump, HIGH);
        pumpStartTime = millis();
        sendLocationSMS();
    } else {
        if (millis() - pumpStartTime > pumpTimeout) {
            digitalWrite(pump, LOW);
        }
        forword();
    }
}

void handleBluetoothCommand(char cmd) {
    switch (cmd) {
        case 'F': forword(); break;
        case 'B': backword(); break;
        case 'L': turnLeft(); break;
        case 'R': turnRight(); break;
        case 'S': Stop(); break;
        case 'P': digitalWrite(pump, !digitalRead(pump)); break;
        default: Stop(); break;
    }
}

void sendLocationSMS() {
    if (gps.location.isValid()) {
        String message = "Fire detected! Location: ";
        message += "https://maps.google.com/?q=";
        message += String(gps.location.lat(), 6);
        message += ",";
        message += String(gps.location.lng(), 6);
        
        gsmSerial.println("AT+CMGF=1");
        delay(1000);
        gsmSerial.print("AT+CMGS=\"");
        gsmSerial.print(phoneNumber);
        gsmSerial.println("\"");
        delay(1000);
        gsmSerial.println(message);
        delay(1000);
        gsmSerial.write(26);
        delay(3000);
    } else {
        Serial.println("GPS signal not available");
    }
}

void forword() {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
}

void backword() {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
}

void turnRight() {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
}

void turnLeft() {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
}

void Stop() {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, LOW);
}
