
#define TRIG_L  2
#define ECHO_L  3
#define TRIG_R  4
#define ECHO_R  5


#define CLOSE_MIN   8    
#define CLOSE_MAX  14    
#define GAP_LOW    15    
#define FAR_MIN    15    
#define FAR_MAX    26    


#define ENTRY_HOLD    120   
                            // eliminates any accidental gesture taht is present in the zone
#define VOL_REPEAT    170   
#define SEEK_REPEAT   220   
#define COOLDOWN       80   

#define NUM_SAMPLES     3   
#define ECHO_TIMEOUT  4500  


enum Zone { NONE, CLOSE, FAR };

struct Sensor {
  uint8_t trig, echo;

  Zone    currentZone;       // the zone where we have currently confirmed the hand in
  Zone    candidateZone;     // zone the hand has just entered inn according to us , originally every gesture is considered to be in candidate zone
  unsigned long enteredAt;   // when hand entered the candidatezone
  unsigned long lastFired;   // when we last sent a command or the sensor last received the command
  unsigned long zoneStartAt; // when hand entered currentZone (for hold detection)
  bool    ppFired;           // so Play/Pause doesn't spam while held
};

Sensor L = { TRIG_L, ECHO_L, NONE, NONE, 0, 0, 0, false };
Sensor R = { TRIG_R, ECHO_R, NONE, NONE, 0, 0, 0, false };


int singlePing(uint8_t trig, uint8_t echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long t = pulseIn(echo, HIGH, ECHO_TIMEOUT);
  if (t == 0) return -1;
  int d = (int)(t * 0.01715f);
  return (d >= CLOSE_MIN && d <= FAR_MAX + 15) ? d : -1;
}

int getDist(Sensor& s) {
  int buf[NUM_SAMPLES], cnt = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    int d = singlePing(s.trig, s.echo);
    if (d >= 0) buf[cnt++] = d;
    delayMicroseconds(350);
  }
  if (cnt == 0) return -1;

  for (int i = 1; i < cnt; i++) {
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j+1] = buf[j]; j--; }
    buf[j+1] = v;
  }
  return buf[cnt / 2];
}


Zone classify(int d) {
  if (d < 0)                          return NONE;
  if (d >= CLOSE_MIN && d <= CLOSE_MAX) return CLOSE;
  if (d >= FAR_MIN   && d <= FAR_MAX)   return FAR;
  return NONE;
}


unsigned long lastAnyCMD = 0;
unsigned long lastPPFired = 0;   
#define POST_PP_QUIET  600  


void sendCMD(Sensor& s, const char* cmd, unsigned long repeatInterval) {
  unsigned long now = millis();
  if (now - lastPPFired   < POST_PP_QUIET)  return;  
  if (now - s.lastFired   < repeatInterval) return;
  if (now - lastAnyCMD    < COOLDOWN)        return;
  Serial.println(cmd);
  s.lastFired = now;
  lastAnyCMD  = now;
}


void updateSensor(Sensor& s, int dist, bool isLeft) {
  unsigned long now  = millis();
  Zone          z    = classify(dist);



  if (z != s.candidateZone) {
   
    s.candidateZone = z;
    s.enteredAt     = now;
  }


  if (z != NONE && z == s.candidateZone &&
      now - s.enteredAt >= ENTRY_HOLD &&
      z != s.currentZone) {
    s.currentZone  = z;
    s.zoneStartAt  = now;
    s.ppFired      = false;   
    s.lastFired    = 0;
  }

  if (z == NONE) {
    s.currentZone = NONE;
    return;
  }

  if (s.currentZone == NONE) return;

  if (isLeft) {
    if (s.currentZone == CLOSE) sendCMD(s, "CMD:VOL_UP",   VOL_REPEAT);
    if (s.currentZone == FAR)   sendCMD(s, "CMD:VOL_DOWN", VOL_REPEAT);

  } else {
    if (s.currentZone == CLOSE) {
      sendCMD(s, "CMD:FORWARD", SEEK_REPEAT);
    }

    if (s.currentZone == FAR) {
      sendCMD(s, "CMD:REWIND", SEEK_REPEAT);
    }
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(TRIG_L, OUTPUT); pinMode(ECHO_L, INPUT);
  pinMode(TRIG_R, OUTPUT); pinMode(ECHO_R, INPUT);
  Serial.println("CMD:READY");
}


#define PP_CONFIRM_MS  300   
#define PP_COOLDOWN   1000   

unsigned long ppOverlapStart = 0;   
bool          ppArmed        = false;
unsigned long lastPP         = 0;

bool inAnyZone(int d) {
  return (d >= CLOSE_MIN && d <= FAR_MAX);
}

void checkPlayPause(int dL, int dR) {
  unsigned long now = millis();

  if (inAnyZone(dL) && inAnyZone(dR)) {
    if (!ppArmed) {
      ppArmed        = true;
      ppOverlapStart = now;     
    } else if (now - ppOverlapStart >= PP_CONFIRM_MS &&
           now - lastPP         >= PP_COOLDOWN) {
  Serial.println("CMD:PLAY_PAUSE");
  lastPP      = now;
  lastPPFired = now;   // ← ADD THIS
  ppArmed     = false;

  // ← ADD THESE — wipe accumulated zone state on both sensors
  L.currentZone   = NONE;  L.candidateZone = NONE;  L.enteredAt = now;
  R.currentZone   = NONE;  R.candidateZone = NONE;  R.enteredAt = now;
}
  } else {
    ppArmed = false;            
  }
}


void loop() {
  int dL = getDist(L);
  int dR = getDist(R);

  
  bool bothIn = inAnyZone(dL) && inAnyZone(dR);
  checkPlayPause(dL, dR);

  if (!bothIn) {
    updateSensor(L, dL, true);
    updateSensor(R, dR, false);
  }

  // Uncomment to debug in Serial Monitor:
  // Serial.print("DBG L="); Serial.print(dL);
  // Serial.print(" R=");    Serial.println(dR);
}
