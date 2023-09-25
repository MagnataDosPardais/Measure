#include <Adafruit_VL53L0X.h>
#include <DHT.h>
#include <DHT_U.h>

#define SERIALSPEED 9600
#define SERIALDELAY 25

#define PZPORT 5
#define BASSNOTE 264
#define MIDNOTE 579
#define TREBLENOTE 1198
#define QNOTES 3
const int NOTES[QNOTES] = {BASSNOTE, MIDNOTE, TREBLENOTE};

#define TOFGPIO 4
VL53L0X_RangingMeasurementData_t measure;
bool debugTof = false;
unsigned int currentValue;
byte readNumb = 20;
unsigned int averageReadBase;
unsigned int averageReadBlock;
unsigned int blockHeight;
unsigned int calVvc = 100; //
int c;
//byte readNumbC = 20;
//int timeoutTof = 50;

#define DHTPORT 3
#define DHTTYPE DHT11
int limDelay;

bool recived = false;
String spltCmd[3];

bool exe = false;
bool cal = false;
byte calStep = 0;


Adafruit_VL53L0X tof = Adafruit_VL53L0X();
DHT_Unified dht(DHTPORT, DHTTYPE);

void setup() {
  Serial.begin(SERIALSPEED);
  Serial.setTimeout(SERIALDELAY);

  for(int s = 0; !Serial; s++) {
    delay(10);
    if(s >= 500) { Serial.println("Load error in Serial"); }
  }
  
  //if(!dht.begin()) { Serial.println("Load error in DHT sensor"); }
  if(!tof.begin()) { Serial.println("Load error in ToF sensor"); }
  pinMode(PZPORT, OUTPUT);
  Sounds_turnOn(PZPORT);
}

void Sounds_turnOn(int port) {
  const int n[3] = {NOTES[1],NOTES[0],NOTES[2]};
  const int p[3][2] = {{240,10},{240,10},{240,10}};
  for(byte i = 0; i < 3; i++) {
    tone(port, n[i]);
    delay(p[i][0]);
    noTone(port);
    delay(p[i][1]);
  }
}

void Cmd_read(String* splited, bool* state, char breakReadIn, char splitIn) {
  String cmd;
  int strCnt = 0;
  int i;
  if(Serial.available() > 0) {
    *state = true;
    cmd = Serial.readStringUntil(breakReadIn);
    cmd.toUpperCase();
    while (cmd.length() > 0) {
      i = cmd.indexOf(splitIn);
      if (i == -1) { splited[strCnt++] = cmd; break; }
      else { splited[strCnt++] = cmd.substring(0, i); cmd = cmd.substring(i + 1); }
    }
    Serial.print("<< "); Serial.print(spltCmd[0]); Serial.print(' '); Serial.print(spltCmd[1]); Serial.print(' '); Serial.println(spltCmd[2]);
  }
}

unsigned int tofRead() {
  tof.rangingTest(&measure, debugTof);
  if(measure.RangeStatus != 4) { return measure.RangeMilliMeter; }
  return(0);
}
void Cmd_clear(String* c) {
  for(byte i = 0; i < 3; i++) { c[i] = ""; }
}

void loop() {
  Cmd_read(spltCmd, &recived, '\n', ' ');

  if(recived && !exe && !cal) {
    
    if(spltCmd[0] == "SVC") {
      if(spltCmd[1] == "START") { exe = true; }
    }
    
    else if(spltCmd[0] == "SYS") {
      if(spltCmd[1] == "-D")        { Serial.print("\tDistância da base: "); Serial.print(spltCmd[2]); Serial.println(F("mm")); } //Att
      else if(spltCmd[1] == "-VVC") { Serial.print("\tParâmetro de calibração: "); Serial.print(spltCmd[2]); Serial.println(F("mm")); calVvc = spltCmd[2].toFloat(); }
      else if(spltCmd[1] == "-RN")  { Serial.print("\tServiço atualizado para "); Serial.print(spltCmd[2]); Serial.println(" leituras"); readNumb =  spltCmd[2].toInt(); }
      //else if(spltCmd[1] == "-RNC")  { Serial.print("\tCalibração atualizada para "); Serial.print(spltCmd[2]); Serial.println(" leituras"); readNumbC = spltCmd[2].toInt(); }

    }
    
    else if(spltCmd[0] == "TOF") {
      if(spltCmd[1] == "-C") { cal = true; }
      else if(spltCmd[1] == "-R") {
        Serial.println("Reading ToF Sensor:");
        for(byte r = 0; r < spltCmd[2].toInt(); r++) {
          Serial.print("\t"); Serial.print(r+1); Serial.print(": "); Serial.println(tofRead());
        }
      }
    }
    
    else if(spltCmd[0] == "DHT") {
      if(spltCmd[1] == "-R") {
        for(int x = spltCmd[2].toInt(); x <= 0; x--) {
          sensors_event_t event;
          if(spltCmd[2] == "T" || spltCmd[2] == "A") {
            dht.temperature().getEvent(&event);
            if(isnan(event.temperature)) { Serial.println(F("Error reading temperature!")); }
            else {
              Serial.print  (F("Temperature: "));
              Serial.print  (event.temperature);
              Serial.println(F("°C"));
            }
          }
          if(spltCmd[2] == "H" || spltCmd[2] == "A") {
            dht.humidity().getEvent(&event);
            if(isnan(event.relative_humidity)) { Serial.println(F("Error reading humidity!")); }
            else {
              Serial.print  (F("Humidity: "));
              Serial.print  (event.relative_humidity);
              Serial.println(F("%"));
            }
          }
        }
      }
      if(spltCmd[1] == "-F") {
        sensor_t sensor;
        if(spltCmd[2] == "T" || spltCmd[2] == "A") {
          dht.temperature().getSensor(&sensor);
          Serial.println(F("------------------------------------"));
          Serial.println(F("Temperature Sensor"));
          Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
          Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
          Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
          Serial.print  (F("Range:       ")); Serial.print(sensor.min_value); Serial.println(F(" ~ ")); Serial.println(sensor.max_value); Serial.println(F("°C"));
          Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
          Serial.println(F("------------------------------------"));
        }
        if(spltCmd[2] == "H" || spltCmd[2] == "A") {
          dht.humidity().getSensor(&sensor);
          Serial.println(F("Humidity Sensor"));
          Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
          Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
          Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
          Serial.print  (F("Range:       ")); Serial.print(sensor.min_value); Serial.println(F(" ~ ")); Serial.println(sensor.max_value); Serial.println(F("%"));
          Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
          Serial.println(F("------------------------------------"));
        }
      }
    }
    else { Serial.print("Comando desconhecido"); }
    recived = false;
  }
  
  else if(exe) {
    if(spltCmd[0] == "D") {
      for(byte x = 0; x < readNumb; x++) {
        Serial.print("\t"); Serial.print(x+1); Serial.print(": "); Serial.print(tofRead()); Serial.println("mm");
      }
    }
    else if(spltCmd[0] == "E") { exe = false; Serial.print("\t"); Serial.print("Service stoped"); }
  }

  else if(cal) { //Error spltCmd[2] empty
    if(calStep == 0) { Serial.println("\tCalibrando sensor ToF ("); Serial.print(spltCmd[2]); Serial.print(")..."); Serial.println("\t\tMedidas da base:\n"); calStep++; }
    int rn = spltCmd[2].toInt();
    if(spltCmd[0] == "D") { calStep++; }
    if(spltCmd[0] == "E") { calStep = 0; cal = false; }
    averageReadBase = 0;
    if(cal == 1) {
      for(byte r = 0; r < rn; r++) {
        unsigned int currentRead = tofRead();
        Serial.print("\t\t\tBase "); Serial.print(r+1); Serial.print(": "); Serial.println(currentRead); Serial.println("mm");
        averageReadBase += currentRead;
      }
      averageReadBase = averageReadBase / rn;
      calStep++;
    }
    if(calStep == 3) { Serial.println("\tCalibrando sensor ToF..."); Serial.println("\t\tMedidas do bloco:\n"); calStep++; }
    else if(calStep == 5) {
      Serial.println("\t\tMedidas do bloco:\n");
      for(byte r = 0; r < rn; r++) {
        unsigned int currentRead = tofRead();
        Serial.print("\t\t\tBloco "); Serial.print(r+1); Serial.print(": "); Serial.println(currentRead); Serial.println("mm");
        averageReadBlock += currentRead;
      }
      calStep = 0;
      cal = false;
    }
    averageReadBlock = averageReadBlock / rn;
    blockHeight = averageReadBase - averageReadBlock;
    c = calVvc - blockHeight;
  }

  Cmd_clear(spltCmd);
  recived = false;
}
