
#define WIFI Serial1   // D1 Mini on pins 18/19

#define S1  22
#define S2  24
#define S3  26
#define S4  28
#define S5  20

#define IN1  11
#define IN2  10
#define IN3  9
#define IN4  8
#define ENA  13
#define ENB  12


#define BASE_SPEED  120
#define TURN_SPEED  170
#define FORWARD_MS  650
#define DEADEND_MS  550

// States
#define EXPLORING 0
#define WAITING   1
#define RUNNING   2
int state = EXPLORING;

// Path storage
char path[100];
int  pathLen = 0;

char runPath[100];
int  runLen = 0;
int  runIdx = 0;

// Motors 

void forward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, BASE_SPEED);
  analogWrite(ENB, BASE_SPEED + 50);
}

void rotateLeft() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, TURN_SPEED);
  analogWrite(ENB, TURN_SPEED + 50);
}

void rotateRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, TURN_SPEED);
  analogWrite(ENB, TURN_SPEED + 50);
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void moveForMs(int ms) {
  forward();
  delay(ms);
  stopMotors();
  delay(50);
}

void turnLeft() {
  analogWrite(ENA, 255); analogWrite(ENB, 255); delay(100);
  while (!(digitalRead(S1)==HIGH && digitalRead(S2)==HIGH && digitalRead(S3)==HIGH &&
           digitalRead(S4)==HIGH && digitalRead(S5)==HIGH)) { rotateLeft(); }
  while (!(digitalRead(S3)==LOW  && digitalRead(S2)==HIGH && digitalRead(S4)==HIGH)) { rotateLeft(); }
  stopMotors();
}

void turnRight() {
  analogWrite(ENA, 255); analogWrite(ENB, 255); delay(100);
  while (!(digitalRead(S1)==HIGH && digitalRead(S2)==HIGH && digitalRead(S3)==HIGH &&
           digitalRead(S4)==HIGH && digitalRead(S5)==HIGH)) { rotateRight(); }
  while (!(digitalRead(S3)==LOW  && digitalRead(S2)==HIGH && digitalRead(S4)==HIGH)) { rotateRight(); }
  stopMotors();
}

void uTurn() {
  analogWrite(ENA, 255); analogWrite(ENB, 255); delay(100);
  while (!(digitalRead(S1)==HIGH && digitalRead(S2)==HIGH && digitalRead(S3)==HIGH &&
           digitalRead(S4)==HIGH && digitalRead(S5)==HIGH)) { rotateRight(); }
  while (!(digitalRead(S3)==LOW  && digitalRead(S2)==HIGH && digitalRead(S4)==HIGH)) { rotateRight(); }
  stopMotors();
}

// WiFi helpers 

void wifiSend(const char* msg) {
  WIFI.println(msg);
  Serial.println(msg);
}

void wifiSendMove(char m) {
  char buf[12];
  sprintf(buf, "MOVE:%c", m);
  WIFI.println(buf);
  Serial.println(buf);
}

// Path optimizer 

char simplify(char a, char c) {
  int angle = 0;
  if (a == 'R') angle = 90;
  else if (a == 'L') angle = 270;
  else if (a == 'U') angle = 180;
  angle = (angle + 180) % 360;
  if (c == 'R') angle = (angle + 90)  % 360;
  else if (c == 'L') angle = (angle + 270) % 360;
  else if (c == 'U') angle = (angle + 180) % 360;
  if (angle == 0)   return 'S';
  if (angle == 90)  return 'R';
  if (angle == 180) return 'U';
  return 'L';
}

void recordMove(char m) {
  path[pathLen++] = m;
  while (pathLen >= 3 && path[pathLen - 2] == 'U') {
    path[pathLen - 3] = simplify(path[pathLen - 3], path[pathLen - 1]);
    pathLen -= 2;
  }
}

// Finish

bool isFinish() {
  return digitalRead(S1)==LOW && digitalRead(S2)==LOW && digitalRead(S3)==LOW &&
         digitalRead(S4)==LOW && digitalRead(S5)==LOW;
}

void handleFinish() {
  stopMotors();

  if (state == RUNNING) {
    wifiSend("DONE:Optimal path complete!");
    while (true);
  }

  // Send optimized path to Python
  WIFI.print("PATH:");
  Serial.print("PATH:");
  for (int i = 0; i < pathLen; i++) {
    WIFI.print(path[i]);
    Serial.print(path[i]);
  }
  WIFI.println();
  Serial.println();

  wifiSend("STATUS:Waiting for RUN command...");
  state = WAITING;
}

//  RUN command

void waitForRun() {
  String line = "";
  while (true) {
    if (WIFI.available()) {
      char c = WIFI.read();
      if (c == '\n' || c == '\r') {
        line.trim();
        if (line.startsWith("RUN:")) {
          String p = line.substring(4);
          runLen = p.length();
          for (int i = 0; i < runLen; i++) runPath[i] = p.charAt(i);
          runIdx = 0;
          state = RUNNING;
          wifiSend("STATUS:Run 2 starting!");
          delay(2000);
          return;
        }
        line = "";
      } else {
        line += c;
      }
    }
  }
}

// Setup 

void setup() {
  pinMode(S1, INPUT); pinMode(S2, INPUT); pinMode(S3, INPUT);
  pinMode(S4, INPUT); pinMode(S5, INPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  Serial.begin(9600);
  WIFI.begin(9600);
  delay(2000);

  wifiSend("STATUS:Ready. Waiting for START...");

  // Wait for START from Python
  String line = "";
  while (true) {
    if (WIFI.available()) {
      char c = WIFI.read();
      if (c == '\n' || c == '\r') {
        line.trim();
        if (line == "START") {
          wifiSend("STATUS:Starting!");
          delay(1000);
          break;
        }
        line = "";
      } else {
        line += c;
      }
    }
  }
}

// Main loop 

void loop() {

  // Waiting for Run 2 command
  if (state == WAITING) {
    waitForRun();
    return;
  }

  int s1 = digitalRead(S1);
  int s2 = digitalRead(S2);
  int s3 = digitalRead(S3);
  int s4 = digitalRead(S4);
  int s5 = digitalRead(S5);

  // 1 — Dead end
  if (s1==HIGH && s2==HIGH && s3==HIGH && s4==HIGH && s5==HIGH) {
    forward();
    delay(DEADEND_MS);
    stopMotors();
    if (digitalRead(S1)==HIGH && digitalRead(S2)==HIGH && digitalRead(S3)==HIGH &&
        digitalRead(S4)==HIGH && digitalRead(S5)==HIGH) {
      wifiSendMove('U');
      uTurn();
      if (state == EXPLORING) recordMove('U');
    }
    return;
  }

  // 2 — Intersection
  if (s3 == LOW && (s1 == LOW || s5 == LOW)) {
    stopMotors();
    delay(100);

    // ── Run 2: follow recorded path ──
    if (state == RUNNING) {
      if (isFinish()) { handleFinish(); return; }
      char cmd = runPath[runIdx++];
      wifiSendMove(cmd);
      if      (cmd == 'L') { moveForMs(FORWARD_MS); if (isFinish()) { handleFinish(); return; } turnLeft();  }
      else if (cmd == 'S') { moveForMs(FORWARD_MS); if (isFinish()) { handleFinish(); return; }              }
      else if (cmd == 'R') { moveForMs(FORWARD_MS); if (isFinish()) { handleFinish(); return; } turnRight(); }
      else                 { uTurn(); }
      return;
    }

    // ── Run 1: left-hand rule ──
    if (digitalRead(S1) == LOW) {
      wifiSendMove('L');
      moveForMs(FORWARD_MS);
      if (isFinish()) { handleFinish(); return; }
      turnLeft();
      recordMove('L');
    } else {
      moveForMs(FORWARD_MS);
      if (isFinish()) { handleFinish(); return; }
      if (digitalRead(S3)==LOW || digitalRead(S2)==LOW || digitalRead(S4)==LOW) {
        wifiSendMove('S');
        recordMove('S');
      } else {
        wifiSendMove('R');
        turnRight();
        recordMove('R');
      }
    }
    return;
  }

  // 3 — On line
  if (s3 == LOW) {
    if (isFinish()) { handleFinish(); return; }
    forward();
    return;
  }

  // 4 — Drifted left
  if (s2 == LOW && s1 == HIGH) {
    while (digitalRead(S3) == HIGH) { rotateLeft(); }
    stopMotors();
    return;
  }

  // 5 — Drifted right
  if (s4 == LOW && s5 == HIGH) {
    while (digitalRead(S3) == HIGH) { rotateRight(); }
    stopMotors();
    return;
  }

  stopMotors();
}
