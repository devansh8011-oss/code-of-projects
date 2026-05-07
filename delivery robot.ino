#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// WiFi AP
const char* ssid = "Nexus";
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
#define LIDAR_SAFE_MM   500
#define ULTRA_SAFE_CM   5

// Robot state
volatile bool emergencyStop = false;

// Commands
struct Command {
  String direction;
  int distance;
};

Command commands[20];
int cmdCount = 0;

// HTML Page with improved UI
const char* htmlPage = R"html(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Nexus - Robot Control</title>
<style>
  * {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
  }

  :root {
    --primary: #00d9ff;
    --secondary: #ff006e;
    --dark-bg: #0a0e27;
    --card-bg: #131a3d;
    --text-primary: #ffffff;
    --text-secondary: #a0aec0;
    --success: #10b981;
    --warning: #f59e0b;
    --danger: #ef4444;
    --border: #1e2749;
  }

  body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: linear-gradient(135deg, var(--dark-bg) 0%, #0f1535 100%);
    color: var(--text-primary);
    overflow-x: hidden;
    min-height: 100vh;
    padding: 10px;
  }

  .container {
    max-width: 500px;
    margin: 0 auto;
  }

  /* Header */
  .header {
    text-align: center;
    padding: 20px 0;
    margin-bottom: 20px;
    border-bottom: 2px solid var(--border);
    position: relative;
    animation: slideDown 0.6s ease-out;
  }

  @keyframes slideDown {
    from {
      opacity: 0;
      transform: translateY(-20px);
    }
    to {
      opacity: 1;
      transform: translateY(0);
    }
  }

  .header h1 {
    font-size: 2.5em;
    font-weight: 900;
    background: linear-gradient(135deg, var(--primary) 0%, var(--secondary) 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    letter-spacing: 2px;
    text-transform: uppercase;
    margin-bottom: 5px;
  }

  .header p {
    color: var(--text-secondary);
    font-size: 0.9em;
    letter-spacing: 1px;
  }

  /* Emergency Stop Button */
  .emergency-stop-container {
    margin-bottom: 20px;
  }

  .btn-emergency {
    width: 100%;
    padding: 16px;
    font-size: 18px;
    font-weight: bold;
    border: 3px solid var(--danger);
    background: linear-gradient(135deg, var(--danger) 0%, #dc2626 100%);
    color: white;
    border-radius: 12px;
    cursor: pointer;
    transition: all 0.3s ease;
    text-transform: uppercase;
    letter-spacing: 1px;
    box-shadow: 0 0 20px rgba(239, 68, 68, 0.3);
    animation: pulse 2s infinite;
  }

  @keyframes pulse {
    0%, 100% {
      box-shadow: 0 0 20px rgba(239, 68, 68, 0.3);
    }
    50% {
      box-shadow: 0 0 30px rgba(239, 68, 68, 0.6);
    }
  }

  .btn-emergency:active {
    transform: scale(0.98);
    box-shadow: 0 0 40px rgba(239, 68, 68, 0.8);
  }

  /* Section */
  .section {
    background: var(--card-bg);
    border: 1px solid var(--border);
    border-radius: 16px;
    padding: 24px;
    margin-bottom: 20px;
    animation: fadeIn 0.6s ease-out;
  }

  @keyframes fadeIn {
    from {
      opacity: 0;
      transform: translateY(10px);
    }
    to {
      opacity: 1;
      transform: translateY(0);
    }
  }

  .section h2 {
    font-size: 1.3em;
    margin-bottom: 16px;
    color: var(--primary);
    text-transform: uppercase;
    letter-spacing: 1px;
  }

  .section h3 {
    font-size: 1em;
    margin-bottom: 12px;
    color: var(--text-secondary);
  }

  /* Direction Grid */
  .direction-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
    margin-bottom: 16px;
  }

  .direction-full {
    grid-column: 1 / -1;
  }

  /* Button Styles */
  .btn {
    padding: 14px 20px;
    font-size: 16px;
    font-weight: 600;
    border: 2px solid var(--primary);
    background: rgba(0, 217, 255, 0.1);
    color: var(--primary);
    border-radius: 10px;
    cursor: pointer;
    transition: all 0.3s ease;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    position: relative;
    overflow: hidden;
  }

  .btn::before {
    content: '';
    position: absolute;
    top: 0;
    left: -100%;
    width: 100%;
    height: 100%;
    background: rgba(0, 217, 255, 0.2);
    transition: left 0.3s ease;
    z-index: -1;
  }

  .btn:hover::before {
    left: 0;
  }

  .btn:active {
    background: var(--primary);
    color: var(--dark-bg);
    transform: scale(0.98);
  }

  .btn-sm {
    padding: 10px 14px;
    font-size: 14px;
    width: 50px;
  }

  .btn-success {
    border-color: var(--success);
    color: var(--success);
    background: rgba(16, 185, 129, 0.1);
  }

  .btn-success:active {
    background: var(--success);
    color: var(--dark-bg);
  }

  .btn-danger {
    border-color: var(--danger);
    color: var(--danger);
    background: rgba(239, 68, 68, 0.1);
  }

  .btn-danger:active {
    background: var(--danger);
    color: white;
  }

  /* Distance Display */
  .distance-display {
    background: linear-gradient(135deg, rgba(0, 217, 255, 0.1), rgba(255, 0, 110, 0.1));
    border: 2px solid var(--primary);
    border-radius: 12px;
    padding: 20px;
    text-align: center;
    margin: 16px 0;
  }

  .distance-display p {
    color: var(--text-secondary);
    font-size: 0.9em;
    margin-bottom: 8px;
  }

  #distVal {
    font-size: 2.8em;
    font-weight: 900;
    color: var(--primary);
    display: block;
    margin: 8px 0;
  }

  .distance-unit {
    color: var(--text-secondary);
    font-size: 1.1em;
  }

  /* Control Grid */
  .control-grid {
    display: grid;
    grid-template-columns: 1fr 1fr 1fr;
    gap: 10px;
    align-items: center;
    margin: 16px 0;
  }

  /* Keyboard Input */
  .keyboard-input {
    background: rgba(0, 217, 255, 0.05);
    border: 2px solid var(--border);
    border-radius: 10px;
    padding: 12px;
    margin: 16px 0;
  }

  #distInput {
    width: 100%;
    padding: 12px;
    font-size: 16px;
    background: rgba(0, 217, 255, 0.1);
    border: 2px solid var(--primary);
    border-radius: 8px;
    color: var(--primary);
    text-align: center;
    font-weight: bold;
  }

  #distInput::placeholder {
    color: var(--text-secondary);
  }

  #distInput:focus {
    outline: none;
    border-color: var(--secondary);
    box-shadow: 0 0 10px rgba(0, 217, 255, 0.3);
  }

  /* Command List */
  #cmdList {
    background: rgba(0, 217, 255, 0.05);
    border: 1px solid var(--border);
    border-radius: 10px;
    padding: 12px;
    min-height: 50px;
    font-size: 13px;
    line-height: 1.6;
    max-height: 200px;
    overflow-y: auto;
  }

  #cmdList::-webkit-scrollbar {
    width: 6px;
  }

  #cmdList::-webkit-scrollbar-track {
    background: rgba(0, 217, 255, 0.05);
    border-radius: 3px;
  }

  #cmdList::-webkit-scrollbar-thumb {
    background: var(--primary);
    border-radius: 3px;
  }

  .cmd-item {
    background: rgba(0, 217, 255, 0.1);
    padding: 8px 12px;
    border-radius: 6px;
    margin-bottom: 6px;
    border-left: 3px solid var(--primary);
  }

  /* Status */
  #status {
    color: var(--success);
    font-size: 13px;
    margin-top: 12px;
    padding: 12px;
    background: rgba(16, 185, 129, 0.1);
    border-left: 3px solid var(--success);
    border-radius: 6px;
    display: none;
  }

  #status.show {
    display: block;
    animation: slideIn 0.3s ease;
  }

  #status.error {
    color: var(--danger);
    background: rgba(239, 68, 68, 0.1);
    border-left-color: var(--danger);
  }

  @keyframes slideIn {
    from {
      opacity: 0;
      transform: translateX(-10px);
    }
    to {
      opacity: 1;
      transform: translateX(0);
    }
  }

  /* Hidden Class */
  .hidden {
    display: none !important;
  }

  /* Direction Label */
  .dir-label {
    background: rgba(0, 217, 255, 0.15);
    border: 1px solid var(--primary);
    border-radius: 8px;
    padding: 12px;
    text-align: center;
    margin: 12px 0;
    font-weight: bold;
    color: var(--primary);
  }

  /* Summary Label */
  .summary-label {
    background: linear-gradient(135deg, rgba(0, 217, 255, 0.15), rgba(255, 0, 110, 0.15));
    border: 2px solid var(--border);
    border-radius: 12px;
    padding: 16px;
    text-align: center;
    margin: 16px 0;
    font-size: 1.1em;
    font-weight: bold;
  }

  /* Button Row */
  .button-row {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 10px;
    margin-top: 16px;
  }

  .button-row.full {
    grid-template-columns: 1fr;
  }

  /* Responsive */
  @media (max-width: 480px) {
    .header h1 {
      font-size: 2em;
    }

    .section {
      padding: 16px;
    }

    .btn {
      padding: 12px 16px;
      font-size: 14px;
    }

    #distVal {
      font-size: 2.2em;
    }
  }
</style>
</head>
<body>

<div class="container">
  <!-- Header -->
  <div class="header">
    <h1>⚡ Nexus</h1>
    <p>Advanced Robot Control System</p>
  </div>

  <!-- Emergency Stop -->
  <div class="emergency-stop-container">
    <button class="btn-emergency" onclick="triggerEmergencyStop()">🛑 EMERGENCY STOP</button>
  </div>

  <!-- Step 1: Direction Selection -->
  <div class="section" id="stepDirection">
    <h2>Select Direction</h2>
    <div class="direction-grid">
      <button class="btn direction-full" onclick="selectDir('FORWARD')">⬆️ Forward</button>
    </div>
    <div class="direction-grid">
      <button class="btn" onclick="selectDir('LEFT')">⬅️ Left</button>
      <button class="btn" onclick="selectDir('RIGHT')">➡️ Right</button>
    </div>
    <div class="direction-grid">
      <button class="btn direction-full" onclick="selectDir('BACKWARD')">⬇️ Backward</button>
    </div>
  </div>

  <!-- Step 2: Distance Selection -->
  <div class="section hidden" id="stepDistance">
    <h2>Set Distance</h2>
    
    <div class="dir-label">
      Direction: <span id="dirLabel"></span>
    </div>

    <div class="distance-display">
      <p>Distance</p>
      <span id="distVal">10</span>
      <span class="distance-unit">cm</span>
    </div>

    <!-- Button Controls -->
    <div class="control-grid">
      <button class="btn btn-sm" onmousedown="startDecrement()" onmouseup="stopIncrement()" ontouchstart="startDecrement()" ontouchend="stopIncrement()">−</button>
      <button class="btn btn-sm" style="visibility: hidden;"></button>
      <button class="btn btn-sm" onmousedown="startIncrement()" onmouseup="stopIncrement()" ontouchstart="startIncrement()" ontouchend="stopIncrement()">+</button>
    </div>

    <!-- Keyboard Input -->
    <div class="keyboard-input">
      <p style="color: var(--text-secondary); font-size: 0.9em; margin-bottom: 8px;">Or enter value directly:</p>
      <input type="number" id="distInput" placeholder="Enter cm" min="5" max="200">
    </div>

    <!-- Actions -->
    <div class="button-row">
      <button class="btn btn-success" onclick="showOptions()">✓ Confirm</button>
      <button class="btn btn-danger" onclick="goBack()">✕ Back</button>
    </div>
  </div>

  <!-- Step 3: Options -->
  <div class="section hidden" id="stepOptions">
    <h2>Execute Command</h2>
    
    <div class="summary-label">
      <span id="summaryLabel"></span>
    </div>

    <div class="button-row full">
      <button class="btn btn-success" onclick="addCommand()">+ Add More Commands</button>
    </div>
    <div class="button-row full">
      <button class="btn" onclick="executeCommands()" style="border-color: var(--primary); grid-column: 1 / -1;">▶️ Execute All</button>
    </div>
    <div class="button-row full">
      <button class="btn btn-danger" onclick="cancelAll()" style="grid-column: 1 / -1;">✕ Cancel All</button>
    </div>
  </div>

  <!-- Command Queue -->
  <div class="section">
    <h3>Command Queue</h3>
    <div id="cmdList">Empty</div>
    <div id="status"></div>
  </div>
</div>

<script>
let selectedDir = '';
let distVal = 10;
let cmdQueue = [];
let incrementInterval = null;
let decrementInterval = null;

function selectDir(dir) {
  selectedDir = dir;
  document.getElementById('dirLabel').innerText = dir;
  distVal = 10;
  document.getElementById('distVal').innerText = distVal;
  document.getElementById('distInput').value = '';
  show('stepDistance');
}

function startIncrement() {
  changeVal(5);
  incrementInterval = setInterval(() => changeVal(5), 150);
}

function startDecrement() {
  changeVal(-5);
  decrementInterval = setInterval(() => changeVal(-5), 150);
}

function stopIncrement() {
  clearInterval(incrementInterval);
  clearInterval(decrementInterval);
}

function changeVal(delta) {
  distVal = Math.max(5, Math.min(200, distVal + delta));
  document.getElementById('distVal').innerText = distVal;
  document.getElementById('distInput').value = distVal;
}

document.getElementById('distInput')?.addEventListener('input', (e) => {
  let val = parseInt(e.target.value);
  if (!isNaN(val)) {
    distVal = Math.max(5, Math.min(200, val));
    document.getElementById('distVal').innerText = distVal;
  }
});

function showOptions() {
  document.getElementById('summaryLabel').innerText = `${selectedDir} → ${distVal} cm`;
  show('stepOptions');
}

function addCommand() {
  cmdQueue.push(selectedDir + ':' + distVal);
  updateList();
  show('stepDirection');
  showStatus('Command added!', 'success');
}

function executeCommands() {
  if (cmdQueue.length === 0 && distVal > 0) {
    cmdQueue.push(selectedDir + ':' + distVal);
  }

  updateList();

  let cmds = cmdQueue.join(',');

  fetch('/execute?cmds=' + encodeURIComponent(cmds))
    .then(r => r.text())
    .then(t => {
      showStatus(t, 'success');
      cmdQueue = [];
      updateList();
      show('stepDirection');
    })
    .catch(e => {
      showStatus('Error: ' + e, 'error');
    });
}

function cancelAll() {
  cmdQueue = [];
  updateList();
  show('stepDirection');
  showStatus('All commands cancelled.', 'success');
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

  if (cmdQueue.length === 0) {
    el.innerHTML = 'Empty';
    return;
  }

  el.innerHTML = cmdQueue.map((c, i) => {
    let parts = c.split(':');
    return `<div class="cmd-item">${i + 1}. ${parts[0]} → ${parts[1]} cm</div>`;
  }).join('');
}

function showStatus(msg, type = 'success') {
  let el = document.getElementById('status');
  el.innerText = msg;
  el.className = 'show ' + (type === 'error' ? 'error' : '');
  setTimeout(() => {
    el.classList.remove('show');
  }, 5000);
}

function triggerEmergencyStop() {
  fetch('/emergency-stop')
    .then(r => r.text())
    .then(t => {
      showStatus('⚠️ EMERGENCY STOP ACTIVATED', 'error');
    });
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
  if (emergencyStop) {
    stopMotors();
    return;
  }

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

    // Buzzer ON if distance is above 5cm (object detected)
    if (dist > 0 && dist > ULTRA_SAFE_CM) {
      stopMotors();
      digitalWrite(BUZZER, HIGH);

      while (getUltrasonicCM() > ULTRA_SAFE_CM) {
        delay(100);
      }

      digitalWrite(BUZZER, LOW);
      return false;
    }
  }

  int lidar = getLidarMM();

  // LIDAR safe distance changed to 500mm
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
  if (emergencyStop) {
    return;
  }

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

  server.send(200, "text/plain", "Executing commands...");

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

void handleEmergencyStop() {
  emergencyStop = true;
  stopMotors();
  server.send(200, "text/plain", "Emergency stop activated");
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
  server.on("/emergency-stop", handleEmergencyStop);
  server.begin();

  beep(200);
  Serial.println("Server started");
}

// ---------- LOOP ----------
void loop() {
  server.handleClient();

  if (emergencyStop) {
    stopMotors();
  }

  long dist = getUltrasonicCM();

  // Buzzer ON if ultrasonic detects something above 5cm
  if (dist > 0 && dist > ULTRA_SAFE_CM) {
    digitalWrite(BUZZER, HIGH);
  } 
  else {
    digitalWrite(BUZZER, LOW);
  }
}
