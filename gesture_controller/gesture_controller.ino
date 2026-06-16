//This is arduino script for wavesynch project 

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

  Zone    currentZone;       // the zone where we hand currently is present in
  Zone    candidateZone;     // zone where hand has just entered  according to us , originally every gesture is considered to be in the candidate zone
  unsigned long enteredAt;   // at what time did the hand enter the candidatezone
  unsigned long lastFired;   // at what time did we last sent a command or the sensor last received the command
  unsigned long zoneStartAt; // when hand entered currentZone (for hold detection)
  bool    ppFired;           // introduced in order to ensure that Play/Pause doesn't spam while hand held constantly in the position
};

//defining the sensors and their pins
Sensor L = { TRIG_L, ECHO_L, NONE, NONE, 0, 0, 0, false };
Sensor R = { TRIG_R, ECHO_R, NONE, NONE, 0, 0, 0, false };


int singlePing(uint8_t trig, uint8_t echo) {
  // Clears the trigger pin of the sensor to ensure a clean HIGH pulse
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  // Sends a 10-microsecond pulse to trigger the sensor burst so that we can measure the distance because of the reflection of the sound wave when any object hits
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long t = pulseIn(echo, HIGH, ECHO_TIMEOUT);
  if (t == 0) return -1; //in case the object is out of range or there is prolonged absence of any interference or any object in front of the sensor
  // Convert time (till which pin stay HIGH) to centimeters using the speed of sound
  int d = (int)(t * 0.01715f);
  // Returns distance if it fits within the zones
  return (d >= CLOSE_MIN && d <= FAR_MAX + 15) ? d : -1;
}

int getDist(Sensor& s) {
  int buf[NUM_SAMPLES], cnt = 0;
  //for sampling and better accuracy or to avoid noise , we gather multiple distance readings
  for (int i = 0; i < NUM_SAMPLES; i++) {
    int d = singlePing(s.trig, s.echo);
    if (d >= 0) buf[cnt++] = d; //only valid readings will be saved
    delayMicroseconds(350); 
  }
  if (cnt == 0) return -1;
//sorts the samples gathered in ascending order
  for (int i = 1; i < cnt; i++) {
    int v = buf[i], j = i - 1;
    while (j >= 0 && buf[j] > v) { buf[j+1] = buf[j]; j--; }
    buf[j+1] = v;
  }
  //best case or the median of all the samples is selected to ensure accuracy 
  return buf[cnt / 2];
}

//to classify which zone the hand is present in 
Zone classify(int d) {
  if (d < 0)                          return NONE;
  if (d >= CLOSE_MIN && d <= CLOSE_MAX) return CLOSE; // Hand is near the sensor
  if (d >= FAR_MIN   && d <= FAR_MAX)   return FAR; // Hand is far from the sensor
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
   
    s.candidateZone = z; // Track new candidate zone
    s.enteredAt     = now; //stars the confirmationtracker 
  }


  if (z != NONE && z == s.candidateZone &&
      now - s.enteredAt >= ENTRY_HOLD &&
      z != s.currentZone) {
    s.currentZone  = z;
    s.zoneStartAt  = now;
    s.ppFired      = false;   
    s.lastFired    = 0;
  }
// Reset zone state if no hand is detected
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
  Serial.println("CMD:READY"); // Signals to the python backend that system is online
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
// Condition met if both sensors are present and working together since for play/pause , both hands are required together
  if (inAnyZone(dL) && inAnyZone(dR)) {
    if (!ppArmed) {
      ppArmed        = true;
      ppOverlapStart = now;     
    } 
    // Trigger if both hands are consistently held for the required time so that we dont calculate an error as a gesture.
    else if (now - ppOverlapStart >= PP_CONFIRM_MS &&
           now - lastPP         >= PP_COOLDOWN) {
  Serial.println("CMD:PLAY_PAUSE");
  lastPP      = now;
  lastPPFired = now;   
  ppArmed     = false;

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


}
