#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// WiFi AP
const char* ssid = "RoboControl";
const char* password = "12345678";
WebServer server(80);

// Motor pins
#define L_RPWM 25
#define L_LPWM 26
#define R_RPWM 27
#define R_LPWM 14
#define L_REN  32
#define L_LEN  33
#define R_REN  12
#define R_LEN  13

// Sensors & Buzzer
#define BUZZER 19
#define TRIG   5
#define ECHO   18

// VL53L0X
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

// Settings
#define MS_PER_CM       8
#define TURN_DURATION   500
#define LIDAR_SAFE_MM   100
#define ULTRA_SAFE_CM   5

// Commands
struct Command {
  String direction;
  int distance;
};

Command commands[20];
int cmdCount = 0;

// HTML Page
const char* htmlPage = R"html(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>RoboControl</title>
<style>
  body { font-family: Arial; text-align: center; background: #1a1a2e; color: white; padding: 20px; }
  h1 { color: #e94560; }
  .btn { background: #16213e; border: 2px solid #e94560; color: white;
         padding: 15px 30px; margin: 8px; font-size: 18px; border-radius: 10px;
         cursor: pointer; width: 140px; }
  .btn:active { background: #e94560; }
  .btn-sm { padding: 10px 20px; font-size: 16px; width: 60px; }
  .btn-green { border-color: #00ff88; }
  .btn-green:active { background: #00ff88; color: black; }
  .btn-red { border-color: #ff4444; }
  .section { margin: 20px auto; max-width: 400px; }
  #distVal { font-size: 32px; font-weight: bold; color: #00ff88; }
  #cmdList { background: #16213e; padding: 10px; border-radius: 10px;
             min-height: 50px; margin: 10px 0; font-size: 14px; }
  .hidden { display: none; }
  #status { color: #00ff88; font-size: 14px; margin-top: 10px; }
</style>
</head>
<body>

<h1>RoboControl</h1>

<div class="section" id="stepDirection">
  <h2>Select Direction</h2>
  <div>
    <button class="btn" onclick="selectDir('FORWARD')">Forward</button>
  </div>
  <div>
    <button class="btn" onclick="selectDir('LEFT')">Left</button>
    <button class="btn" onclick="selectDir('RIGHT')">Right</button>
  </div>
  <div>
    <button class="btn" onclick="selectDir('BACKWARD')">Backward</button>
  </div>
</div>

<div class="section hidden" id="stepDistance">
  <h2>Set Distance (cm)</h2>
  <p>Direction: <b id="dirLabel"></b></p>
  <button class="btn btn-sm" onclick="changeVal(-5)">-</button>
  <span id="distVal">10</span> cm
  <button class="btn btn-sm" onclick="changeVal(5)">+</button>
  <br><br>
  <button class="btn btn-green" onclick="showOptions()">Confirm</button>
  <button class="btn btn-red" onclick="goBack()">Back</button>
</div>

<div class="section hidden" id="stepOptions">
  <h2>What next?</h2>
  <p><b id="summaryLabel"></b></p>
  <button class="btn btn-green" onclick="addCommand()">Add More</button>
  <button class="btn" onclick="executeCommands()">Execute</button>
  <button class="btn btn-red" onclick="cancelAll()">Cancel All</button>
</div>

<div class="section">
  <h3>Command Queue:</h3>
  <div id="cmdList">Empty</div>
  <p id="status"></p>
</div>

<script>
let selectedDir = '';
let distVal = 10;
let cmdQueue = [];

function selectDir(dir) {
  selectedDir = dir;
  document.getElementById('dirLabel').innerText = dir;
  distVal = 10;
  document.getElementById('distVal').innerText = distVal;
  show('stepDistance');
}

function changeVal(delta) {
  distVal = Math.max(5, distVal + delta);
  document.getElementById('distVal').innerText = distVal;
}

function showOptions() {
  document.getElementById('summaryLabel').innerText = selectedDir + ' ' + distVal + ' cm';
  show('stepOptions');
}

function addCommand() {
  cmdQueue.push(selectedDir + ':' + distVal);
  updateList();
  show('stepDirection');
}

function executeCommands() {
  cmdQueue.push(selectedDir + ':' + distVal);
  updateList();

  let cmds = cmdQueue.join(',');

  fetch('/execute?cmds=' + encodeURIComponent(cmds))
    .then(r => r.text())
    .then(t => {
      document.getElementById('status').innerText = t;
      cmdQueue = [];
      updateList();
      show('stepDirection');
    });
}

function cancelAll() {
  cmdQueue = [];
  updateList();
  show('stepDirection');
  document.getElementById('status').innerText = 'Cancelled.';
}

function goBack() {
  show('stepDirection');
}

function show(id) {
  document.getElementById('stepDirection').classList.add('hidden');
  document.getElementById('stepDistance').classList.add('hidden');
  document.getElementById('stepOptions').classList.add('hidden');
  document.getElementById(id).classList.remove('hidden');
}

function updateList() {
  let el = document.getElementById('cmdList');

  if (cmdQueue.length == 0) {
    el.innerText = 'Empty';
    return;
  }

  el.innerText = cmdQueue.map((c, i) => (i + 1) + '. ' + c.replace(':', ' -> ') + ' cm').join('\n');
}
</script>

</body>
</html>
)html";

// ---------- MOTOR CONTROL ----------
void stopMotors() {
  analogWrite(L_RPWM, 0);
  analogWrite(L_LPWM, 0);
  analogWrite(R_RPWM, 0);
  analogWrite(R_LPWM, 0);
}

void setDirection(String dir, int spd) {
  if (dir == "FORWARD") {
    analogWrite(L_RPWM, spd);
    analogWrite(L_LPWM, 0);
    analogWrite(R_RPWM, spd);
    analogWrite(R_LPWM, 0);
  } 
  else if (dir == "BACKWARD") {
    analogWrite(L_RPWM, 0);
    analogWrite(L_LPWM, spd);
    analogWrite(R_RPWM, 0);
    analogWrite(R_LPWM, spd);
  } 
  else if (dir == "LEFT") {
    analogWrite(L_RPWM, 0);
    analogWrite(L_LPWM, spd);
    analogWrite(R_RPWM, spd);
    analogWrite(R_LPWM, 0);
  } 
  else if (dir == "RIGHT") {
    analogWrite(L_RPWM, spd);
    analogWrite(L_LPWM, 0);
    analogWrite(R_RPWM, 0);
    analogWrite(R_LPWM, spd);
  }
}

// ---------- BUZZER ----------
void beep(int ms = 100) {
  digitalWrite(BUZZER, HIGH);
  delay(ms);
  digitalWrite(BUZZER, LOW);
}

// ---------- ULTRASONIC ----------
long getUltrasonicCM() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 30000);
  return duration / 58;
}

// ---------- LIDAR ----------
int getLidarMM() {
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {
    return measure.RangeMilliMeter;
  }

  return 9999;
}

// ---------- SAFETY CHECK ----------
bool safetyCheck(String dir) {
  if (dir == "BACKWARD") {
    long dist = getUltrasonicCM();

    if (dist > 0 && dist < ULTRA_SAFE_CM) {
      stopMotors();
      digitalWrite(BUZZER, HIGH);

      while (getUltrasonicCM() < ULTRA_SAFE_CM) {
        delay(100);
      }

      digitalWrite(BUZZER, LOW);
      return false;
    }
  }

  int lidar = getLidarMM();

  if (lidar < LIDAR_SAFE_MM) {
    stopMotors();
    beep(500);

    while (getLidarMM() < LIDAR_SAFE_MM) {
      delay(100);
    }

    return false;
  }

  return true;
}

// ---------- EXECUTE ONE COMMAND ----------
void executeOne(String dir, int dist_cm) {
  beep(100);

  if (dir == "LEFT" || dir == "RIGHT") {
    setDirection(dir, 180);
    delay(TURN_DURATION);
    stopMotors();
    delay(200);

    int duration = dist_cm * MS_PER_CM;
    unsigned long start = millis();

    setDirection("FORWARD", 180);

    while (millis() - start < duration) {
      if (!safetyCheck("FORWARD")) {
        unsigned long elapsed = millis() - start;
        setDirection("FORWARD", 180);
        start = millis() - elapsed;
      }
    }

    stopMotors();
  } 
  else {
    int duration = dist_cm * MS_PER_CM;
    unsigned long start = millis();

    setDirection(dir, 180);

    while (millis() - start < duration) {
      if (!safetyCheck(dir)) {
        unsigned long elapsed = millis() - start;
        setDirection(dir, 180);
        start = millis() - elapsed;
      }
    }

    stopMotors();
  }
}

// ---------- WEB HANDLERS ----------
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleExecute() {
  if (!server.hasArg("cmds")) {
    server.send(400, "text/plain", "No commands");
    return;
  }

  server.send(200, "text/plain", "Executing...");

  String cmds = server.arg("cmds");

  while (cmds.length() > 0) {
    int comma = cmds.indexOf(',');
    String token = (comma == -1) ? cmds : cmds.substring(0, comma);
    cmds = (comma == -1) ? "" : cmds.substring(comma + 1);

    int colon = token.indexOf(':');

    if (colon == -1) {
      continue;
    }

    String dir = token.substring(0, colon);
    int dist = token.substring(colon + 1).toInt();

    executeOne(dir, dist);
    delay(200);
  }

  beep(300);
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);

  pinMode(L_RPWM, OUTPUT);
  analogWrite(L_RPWM, 0);

  pinMode(L_LPWM, OUTPUT);
  analogWrite(L_LPWM, 0);

  pinMode(R_RPWM, OUTPUT);
  analogWrite(R_RPWM, 0);

  pinMode(R_LPWM, OUTPUT);
  analogWrite(R_LPWM, 0);

  pinMode(L_REN, OUTPUT);
  digitalWrite(L_REN, HIGH);

  pinMode(L_LEN, OUTPUT);
  digitalWrite(L_LEN, HIGH);

  pinMode(R_REN, OUTPUT);
  digitalWrite(R_REN, HIGH);

  pinMode(R_LEN, OUTPUT);
  digitalWrite(R_LEN, HIGH);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  Wire.begin(21, 22);
  Wire.setClock(100000);
  delay(500);

  if (!lox.begin()) {
    Serial.println("Failed to start VL53L0X");
  }

  WiFi.softAP(ssid, password);

  Serial.println("AP started");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/execute", handleExecute);
  server.begin();

  beep(200);
  Serial.println("Server started");
}

// ---------- LOOP ----------
void loop() {
  server.handleClient();

  long dist = getUltrasonicCM();

  if (dist > 0 && dist < ULTRA_SAFE_CM) {
    digitalWrite(BUZZER, HIGH);
  } 
  else {
    digitalWrite(BUZZER, LOW);
  }
}
