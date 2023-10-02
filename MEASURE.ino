/* PROJETO DE METROLOGIA MECÂNICA
  *  
  * DIAGRAMA DE PORTAS
  *   - Arduino Nano
  *   - VL53L0X = {SCL: A5, SDA: A4, GPIO1: D4, XSHUT: --}
  *   - DHT11   = {DATA: D3}
  *   - Piezo   = {+: D5}
  * 
  * LISTA DE COMANDOS:
  *   -> SVC START: Inicia o serviço da máquina
  *     -> D: Efetua a medição
  *     -> E: Interrompe o serviço
  *   -> SYS: Altera as variáveis de sistema
  *     -> -VVC [uint]: Altera o valor verdadeiro convencionado do parâmetro de calibração para [uint]
  *     -> -RN [byte]: Altera o número de leituras por execução para [byte]
  *     -> -_VVC: Visualiza o valor verdadeiro convencionado do parâmetro de calibração
  *     -> -_RN: Visualiza o número de leituras por execução
  *     -> -_C: Visualiza o valor da correção
  *   -> TOF: Executa uma operação no sensor VL53L0X
  *     -> -C [byte]: Realiza a calibração do sensor com [byte] medições
  *       -> D: Efetua a medição
  *       -> E: Interrompe o serviço
  *     -> -M [byte]: Realiza [byte] medições instantâneas corrigidas
  *     -> -R [byte]: Realiza [byte] medições instantâneas cruas
  *   -> DHT: Executa uma operação no sensor DHT11 ~(Desativado por padrão devido ao armazenamento)
  *     -> -R ['H','A','T']: Realiza medições instantâneas da ['T','A'] == Temperatura, ['H','A'] == Umidade
  *     -> -F: Apresenta as features (características) do sensor
  * 
  * DESENVOLVIDO POR:
  *   -> Adiel Assis Menezes da Silva
  *   -> Leonardo Kenji Ueze
  *   -> Marco Antônio Rocha
  *   -> Marco Antônio Zerbielli Bee
*/

//Bibliotecas
#include <Adafruit_VL53L0X.h> //Biblioteca do sensor VL53L0X
#include <DHT.h>              //Biblioteca do sensor DHT
#include <DHT_U.h>

#define SERIALSPEED 9600  //Velocidade do Serial
#define SERIALDELAY 25    //Delay do Serial

#define PZPORT 5                        //Pino Piezo
const int NOTES[3] = {264, 579, 1198};  //Notas do Piezo

//#define TOFGPIO 4                       //Pino do GPIO1 do VL53L0X
VL53L0X_RangingMeasurementData_t measure; //Struct do VL53L0X
bool debugTof = false;                    //Ativação do modo debug
byte readNumb = 20;                       //Quantidade de leituras do serviço
byte readNumbC = 20;                      //Quantidade de leituras da calibração
unsigned int averageReadBase;             //Média da distância Sensor-Base
unsigned int averageReadBlock;            //Média da distância Sensor-Bloco
unsigned int blockHeight;                 //Altura do bloco
unsigned int calVvc = 100;                //Valor verdadeiro convencionado do bloco
int c = 0;                                //Valor da correção
// char temperature;
// byte humidity;

#define DHTPORT 3     //Pino DHT
#define DHTTYPE DHT11 //Modelo do sensor DHT

bool recived = false; //Controle de comando recebido
bool exe = false;     //Controle de execução de serviço
bool cal = false;     //Controle de execução de calibração
byte calStep = 0;     //Etapa da calibração
String spltCmd[3];    //Comando recebido

Adafruit_VL53L0X tof = Adafruit_VL53L0X();  //Criação do objeto do VL53L0X
DHT_Unified dht(DHTPORT, DHTTYPE);          //Criação do objeto do DHT


void Sounds_turnOn(int port) {  //Som executado ao ser ligado
  const int n[3] = {NOTES[1],NOTES[0],NOTES[2]};
  const int p[3][2] = {{240,10},{240,10},{240,10}};
  for(byte i = 0; i < 3; i++) {
    tone(port, n[i]);
    delay(p[i][0]);
    noTone(port);
    delay(p[i][1]);
  }
}

void Cmd_read(String* splited, bool* state, char breakReadIn, char splitIn) { //Ler o Serial
  String cmd;
  int strCnt = 0;
  if(Serial.available() > 0) {
    int i;
    *state = true;
    cmd = Serial.readStringUntil(breakReadIn);
    cmd.toUpperCase();
    Serial.print(F("<< ")); Serial.println(cmd);
    while (cmd.length() > 0) {
      i = cmd.indexOf(splitIn);
      if(i == -1) { splited[strCnt++] = cmd; break; }
      else { splited[strCnt++] = cmd.substring(0, i); cmd = cmd.substring(i + 1); }
    }
  }
}

void Cmd_clear(String* c) { for(byte i = 0; i < 3; i++) { c[i] = ""; } }  //Limpar a variável do Serial

unsigned int tofRead(byte correction) { //Leitura do sensor ToF
  tof.rangingTest(&measure, debugTof);
  if(measure.RangeStatus != 4) { return measure.RangeMilliMeter + correction; }
  return(0);
}

void tofPrint(String s, byte r, unsigned int v) { //Escrita do sensor ToF
  Serial.print(s);
  Serial.print(F("\t"));
  Serial.print(r+1);
  Serial.print(F(": "));
  Serial.print(tofRead(c));
  Serial.println(F("mm"));
}


void setup() {
  //Inicialização do Serial
  Serial.begin(SERIALSPEED);
  Serial.setTimeout(SERIALDELAY);
  delay(600);
  Serial.println("Pronto[Serial]");

  //Inicialização dos Sensores e portas
  dht.begin();
  for(byte s = 0; s < 30; s++) { if(tof.begin()) { break; } }
  tof.begin() ? Serial.println(F("Pronto[ToF]")) : Serial.println(F("Erro[ToF]"));
  DDRD = B00100000;
  Serial.println(F("Setup pronto"));
  Sounds_turnOn(PZPORT);
}


void loop() {
  Cmd_read(spltCmd, &recived, '\n', ' '); //Ler o Serial

  if(recived && !exe && !cal) { //Escopo de comandos *Funções no cabeçalho*
    if(spltCmd[0] == "SVC") { if(spltCmd[1] == "START") { exe = true; } }
    else if(spltCmd[0] == "SYS") {
      if(spltCmd[1] == "-VVC") { 
        Serial.print(F("\tVVC -> ")); Serial.print(spltCmd[2]); Serial.println(F("mm"));
        calVvc = spltCmd[2].toInt();
      }
      else if(spltCmd[1] == "-_VVC") {  Serial.print(F("\tVVC == ")); Serial.print(spltCmd[2]); Serial.println(F("mm")); }
      else if(spltCmd[1] == "-RN") {
        Serial.print(F("\tRN -> ")); Serial.print(spltCmd[2]); Serial.println(F("x"));
        readNumb = spltCmd[2].toInt();
      }
      else if(spltCmd[1] == "-_RN") { Serial.print(F("\tRN == ")); Serial.print(spltCmd[2]); Serial.println(F("x")); }
      else if(spltCmd[1] == "-_C") { Serial.print(F("\tC == ")); Serial.print(c); Serial.println(F("mm")); }
    }
    else if(spltCmd[0] == "TOF") {
      if(spltCmd[1] == "-C") { cal = true; readNumbC = spltCmd[2].toInt(); }
      else if(spltCmd[1] == "-M") {
        Serial.println(F("Lendo ToF:"));
        for(byte r = 0; r < spltCmd[2].toInt(); r++) { tofPrint("\t\t", r+1, tofRead(c)); }
      }
      else if(spltCmd[1] == "-R") {
        Serial.println(F("Lendo ToF:"));
        for(byte r = 0; r < spltCmd[2].toInt(); r++) { tofPrint("\t\t", r+1, tofRead(0)); }
      }
    }
    /*else if(spltCmd[0] == "DHT") {
      if(spltCmd[1] == "-R") {
        for(int x = spltCmd[2].toInt(); x <= 0; x--) {
          sensors_event_t event;
          if(spltCmd[2] == "T" || spltCmd[2] == "A") {
            dht.temperature().getEvent(&event);
            if(isnan(event.temperature)) { Serial.println(F("Error reading temperature!")); }
            else { Serial.print("Temperatura: "); Serial.print(event.temperature); Serial.println("°C"); }
          }
          if(spltCmd[2] == "H" || spltCmd[2] == "A") {
            dht.humidity().getEvent(&event);
            if(isnan(event.relative_humidity)) { Serial.println(F("Error reading humidity!")); }
            else { Serial.print("Humidity: "); Serial.print(event.relative_humidity); Serial.println("%"); }
          }
        }
      }
      else if(spltCmd[1] == "-F") {
        sensor_t sensor;
        if(spltCmd[2] == "T" || spltCmd[2] == "A") {
          dht.temperature().getSensor(&sensor);
          Serial.println("------------------------------------");
          Serial.println("Sensor de Temperatura");
          Serial.print  ("Sensor:      "); Serial.println(sensor.name);
          Serial.print  ("Driver Ver:  "); Serial.println(sensor.version);
          Serial.print  ("ID Único:    "); Serial.println(sensor.sensor_id);
          Serial.print  ("Intervalo:   "); Serial.print(sensor.min_value); Serial.println(" ~ "); Serial.println(sensor.max_value); Serial.println("°C");
          Serial.print  ("Resolução:   "); Serial.print(sensor.resolution); Serial.println("°C");
          Serial.println("------------------------------------");
        }
        if(spltCmd[2] == "H" || spltCmd[2] == "A") {
          dht.humidity().getSensor(&sensor);
          Serial.println("Sensor de Humidade");
          Serial.print  ("Sensor:     "); Serial.println(sensor.name);
          Serial.print  ("Driver Ver: "); Serial.println(sensor.version);
          Serial.print  ("ID Único:   "); Serial.println(sensor.sensor_id);
          Serial.print  ("Intervalo:  "); Serial.print(sensor.min_value); Serial.println(" ~ "); Serial.println(sensor.max_value); Serial.println("%");
          Serial.print  ("Resolução:  "); Serial.print(sensor.resolution); Serial.println("%");
          Serial.println("------------------------------------");
        }
      }
    }*/
  }

  else if(exe) {  //Ciclo de execução
    if(spltCmd[0] == "D") {
      for(byte x = 0; x < readNumb; x++) { tofPrint("\t\t", x+1, tofRead(c)); }
    }
    else if(spltCmd[0] == "E") {
      exe = false; Serial.print(F("\t")); Serial.print(F("Serviço parado"));
    }
  }

  else if(cal) {  //Ciclo de calibração
    if(spltCmd[0] == "D") { calStep++; }
    if(spltCmd[0] == "E") { Serial.println(F("Calibração parada")); calStep = 0; cal = false; }
    averageReadBase = 0;
    if(calStep == 0 || calStep == 3) {  //Aviso de calibração
      Serial.print(F("\tCalibrar ToF -> ")); Serial.println(readNumbC);
      calStep++;
    }
    else if(calStep == 2) {             //Medidas da base
      Serial.println(F("\t\tMedidas [base]:"));
      for(byte r = 0; r < readNumbC; r++) {
        unsigned int currentRead = tofRead(0);
        tofPrint("\t\t\tBase ", r+1, tofRead(0));
        averageReadBase += currentRead;
      }
      averageReadBase = averageReadBase / readNumbC;
      calStep++;
    }
    else if(calStep == 5) {             //Medidas do bloco
      Serial.println(F("\t\tMedidas [bloco]:"));
      for(byte r = 0; r < readNumbC; r++) {
        unsigned int currentRead = tofRead(0);
        tofPrint("\t\t\tBloco ", r+1, tofRead(0));
        averageReadBlock += currentRead;
      }
      Serial.println(F("\tFim da Calibração:"));
      //Atualizar as variáveis
      calStep = 0;
      cal = false;
      averageReadBlock = averageReadBlock / readNumbC;
      blockHeight = averageReadBase - averageReadBlock;
      c = calVvc - blockHeight;

      //Valores das leituras
      Serial.print(F("\t\tBase:  ")); Serial.print(averageReadBase); Serial.println(F("mm"));
      Serial.print(F("\t\tBloco: ")); Serial.print(averageReadBlock); Serial.println(F("mm"));
      Serial.print(F("\t\tCorr:  ")); Serial.print(c); Serial.println(F("mm"));

      //Leitura da temperatura e umidade
      sensors_event_t event;
      dht.temperature().getEvent(&event);
      char temperature = event.temperature;
      dht.humidity().getEvent(&event);
      int humidity = event.relative_humidity;
      if(isnan(temperature)) { Serial.println(F("Erro[Temp]")); }
      else { Serial.print(F("Temp: ")); Serial.print(temperature,DEC); Serial.println(F("°C")); }
      if(isnan(humidity)) { Serial.println(F("Erro[Umid]")); }
      else { Serial.print("Umid: "); Serial.print(humidity); Serial.println(F("%")); }
    }
  }

  //Reinicia as variáveis de comando
  Cmd_clear(spltCmd);
  recived = false;
}
