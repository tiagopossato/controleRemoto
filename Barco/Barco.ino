/**
   https://github.com/tiagopossato/CC1101Radio

*/

#include "SPI.h"
#include "CC1101Radio.h"
#include <CheapStepper.h>
//#include <TimerOne.h>
#include <MsTimer2.h>

#include <Servo.h>


#define CONTROLE 1
#define BARCO 2

#define MOTOR_PWM 6
#define CANHAO 4
#define SERVO 5
#define MOTOR_FRENTE 3
#define MOTOR_TRAS 7
#define BUZINA 8

#define BATERIA 14

#define MP1 15
#define MP2 16
#define MP3 17
#define MP4 18
#define SENSOR_LEME 19

CheapStepper stepper( MP1, MP2, MP3, MP4);
Servo myservo;
int portSpeak(BUZINA);
byte melodia[] = {240, 239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 226, 225, 224, 223, 222, 221, 220, 219, 218, 217, 216, 215, 214, 213, 212, 211, 210, 209, 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 194, 193, 192, 191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176, 175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 165, 164, 163, 162, 161, 160, 159, 158, 157, 156, 155, 154, 153, 152, 151, 150};

//duraçao de cada nota
uint16_t duracaodasnotas[] = {30, 30, 30, 30, 30, 30, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 5000};

byte melodiaCanhao[] = {40, 40, 40, 40, 50, 40, 40, 40, 40, 40, 50, 40, 40, 40, 40, 50, 40, 40, 40, 40, 40, 40};

//duraçao de cada nota
byte duracaodasnotasCanhao[] = {25, 25, 25, 25, 25, 25, 25, 25, 25, 20, 20, 20, 25, 25, 25, 25, 25, 25, 25, 20};

//estrutura com os dados recebidos
struct {
  uint16_t leme;
  uint16_t motor;
  byte servo;
  byte buzina;
  byte canhao;
} controle;

unsigned char lemePos = 0;
//estrutura com os dados a serem enviados
struct {
  byte bateria;
} barco;

CC1101Radio cc1101;

// ----------------------
// Contadores e Timers
// ----------------------
uint32_t msUltimoEnvio = 0;
#define msEntreEnvios 2000

// ----------------------
// Funções de interrrupção
// ----------------------
void cc1101signalsInterrupt(void) {
  cc1101.packetAvailable = true;
}

void attachIntr() {
  attachInterrupt(digitalPinToInterrupt(cc1101.GDO0pin), cc1101signalsInterrupt, FALLING);
}

void detachIntr() {
  detachInterrupt(digitalPinToInterrupt(cc1101.GDO0pin));
}

// ----------------------
// Setup() & Loop()
// ----------------------

void setup()
{
  Serial.begin(9600);

  pinMode(BATERIA, INPUT);

  pinMode(MOTOR_PWM, OUTPUT);
  pinMode(CANHAO, OUTPUT);
  pinMode(MOTOR_FRENTE, OUTPUT);
  pinMode(MOTOR_TRAS, OUTPUT);
  pinMode(BUZINA, OUTPUT);

  //inicializa o radio com os valores padrao
  cc1101.init();

  //define o endereco local do radio
  cc1101.deviceData.deviceAddress = BARCO;

  //define o endereço remoto
  cc1101.deviceData.remoteDeviceAddress = CONTROLE;

  //Configura o rádio para filtrar os dados recebidos e aceitar somente
  //pacotes enviados para este endereço local
  cc1101.deviceData.addressCheck = true;

  //Configura os parâmetros da comunicação
  //Frequencia de operação
  cc1101.deviceData.carrierFreq = CFREQ_433;
  //potência de transmissão
  cc1101.deviceData.txPower = PA_MaxDistance;
  //canal
  cc1101.deviceData.channel = 5 ;
  //configurações de sincronização
  cc1101.deviceData.syncWord[0] = 19;
  cc1101.deviceData.syncWord[1] = 9;

  // write deviceData to cc1101 module
  cc1101.begin();

  // start reading the GDO0 pin through the ISR function
  attachIntr();

  stepper.setRpm(10);
  stepper.setTotalSteps(4096);
  calibraLeme();

  MsTimer2::set(1, rodaMotor); // 500ms period
  MsTimer2::start();
  //
  //  Timer1.initialize(1000);
  //  Timer1.attachInterrupt(rodaMotor); // blinkLED to run every 0.15 seconds


  myservo.attach(SERVO);

}

void loop() {

  //-------INICIO DA COMUNICAÇÃO SEM FIO------------
  // Verifica se o radio recebeu um pacote
  if (!pacoteRecebido()) {
    // Se não recebeu um pacote, verifica se está na hora de enviar os dados
    if (millis() > msUltimoEnvio + msEntreEnvios) {
      enviaDados();
      msUltimoEnvio = millis();
    }
  } else {
    mostraDados();
  }
  //-------FIM DA COMUNICAÇÃO SEM FIO------------

  barco.bateria = digitalRead(BATERIA);


  myservo.write(controle.servo);

  if (controle.buzina == 1) {
    buzina();
  }
  if (controle.canhao == 1) {
    digitalWrite(CANHAO, 1);
    canhao();
  }
  if (controle.canhao == 0) {
    digitalWrite(CANHAO, 0);
  }
  ponteh();

}


void calibraLeme() {
  while (digitalRead(SENSOR_LEME) == false) {
    stepper.step(false);
  }
  stepper.moveDegrees(true, 90);
  lemePos = 90;
}

void rodaMotor() {

  if (controle.leme < 205) {

    if (lemePos < 15) {
      stepper.moveDegrees(true, 1);
      lemePos++;
    }
    if (lemePos > 15) {
      stepper.moveDegrees(false, 1);
      lemePos--;
    }
  }
  if (controle.leme > 205 && controle.leme < 410) {
    if (lemePos < 65) {
      stepper.moveDegrees(true, 1);
      lemePos++;
    }
    if (lemePos > 65) {
      stepper.moveDegrees(false, 1);
      lemePos--;
    }
  }

  if (controle.leme > 410 && controle.leme < 615) {
    if (lemePos < 90) {
      stepper.moveDegrees(true, 1);
      lemePos++;
    }
    if (lemePos > 90) {
      stepper.moveDegrees(false, 1);
      lemePos--;
    }
  }
  if (controle.leme > 615 && controle.leme < 820) {
    if (lemePos < 130) {
      stepper.moveDegrees(true, 1);
      lemePos++;
    }
    if (lemePos > 130) {
      stepper.moveDegrees(false, 1);
      lemePos--;
    }

  }
  if (controle.leme > 820) {

    if (lemePos < 165) {
      stepper.moveDegrees(true, 1);
      lemePos++;
    }
    if (lemePos > 165) {
      stepper.moveDegrees(false, 1);
      lemePos--;
    }

  }
}


void mostraDados() {

  Serial.print("barco.bateria: ");
  Serial.println(barco.bateria);

  Serial.println("------------------------------------");
  Serial.print("controle.leme: ");
  Serial.println(controle.leme);

  Serial.print("controle.motor: ");
  Serial.println(controle.motor);

  Serial.print("controle.servo: ");
  Serial.println(controle.servo);

  Serial.print("controle.buzina: ");
  Serial.println(controle.buzina);

  Serial.print("controle.canhao: ");
  Serial.println(controle.canhao);
  Serial.println("------------------------------------\nBARCO\n");
}
// ----------------------
// Send & Receive
// ----------------------

void enviaDados() {
  //tamanho do pacote que sera enviado
  uint8_t dataLength = 3;

  //cria pacote
  CC1101Radio::CCPACKET pkt;

  //define o tamanho
  pkt.length = dataLength;

  //monta o pacote
  pkt.data[0] = cc1101.deviceData.remoteDeviceAddress;
  pkt.data[1] = cc1101.deviceData.deviceAddress;
  pkt.data[2] = barco.bateria;

  // transmite os dados
  cc1101.sendData(pkt);
}

bool pacoteRecebido() {

  if (!cc1101.packetAvailable) {
    return false;
  }

  // O modulo recebeu dados

  //desliga a interrupção externa
  detachIntr();

  //muda a variável para false
  cc1101.packetAvailable = false;

  //cria um pacote para receber os dados
  CC1101Radio::CCPACKET pkt;

  // Le o pacote e verifica se conseguiu ler
  if (cc1101.receiveData(&pkt)) {

    // Verifica se o pacote é valido
    if ((pkt.crc_ok == 1) && (pkt.length > 0)) {

      //Serial.println("\r\nNovos dados recebidos:");

      //pega os dados do pacote recebido e altera os valores da estrutura
      controle.leme = word(pkt.data[3], pkt.data[2]);
      controle.motor = word(pkt.data[5], pkt.data[4]);
      controle.servo = pkt.data[6];
      controle.buzina = pkt.data[7];
      controle.canhao = pkt.data[8];
    } // crc & len>0
  } // cc1101.readData

  //Liga novamente a interrupção para permitir novos dados
  attachIntr();

  return true;
}

void buzina() {
  uint16_t pausadepoisdasnotas[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5000};

  for (int nota = 0; nota < 91; nota++) {
    uint16_t duracaodanota = duracaodasnotas[nota];
    tone(portSpeak, melodia[nota], duracaodanota);
    //pausa depois das notas
    delay(pausadepoisdasnotas[nota]);
  }

  noTone(portSpeak);


}

void canhao() {


  byte pausadepoisdasnotas[] = {60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60};

  for (int nota = 0; nota < 20; nota++) {
    byte duracaodanota = duracaodasnotasCanhao[nota];
    tone(portSpeak, melodiaCanhao[nota], duracaodanota);
    //pausa depois das notas
    delay(pausadepoisdasnotas[nota]);
  }

  noTone(portSpeak);

}

void ponteh() {

  if (controle.motor < 500) {
    digitalWrite(MOTOR_TRAS, 1);
    digitalWrite(MOTOR_FRENTE, 0);
  analogWrite(MOTOR_PWM, map(controle.motor, 0, 500, 255, 0));
   //analogWrite(MOTOR_PWM, 128);
  }
  if (controle.motor > 600) {
    digitalWrite(MOTOR_FRENTE, 1);
    digitalWrite(MOTOR_TRAS, 0);
   analogWrite(MOTOR_PWM, map(controle.motor, 600, 1065, 0, 255));
   //analogWrite(MOTOR_PWM,128);
  }
 else if (controle.motor > 501 && controle.motor < 599) {
    digitalWrite(MOTOR_TRAS, 0);
    digitalWrite(MOTOR_FRENTE, 0);
    analogWrite(MOTOR_PWM, 0);
  }

}



