/**
   https://github.com/tiagopossato/CC1101Radio

*/
#include "SPI.h"
#include "CC1101Radio.h"
#define CONTROLE 1
#define BARCO 2

#define AMOSTRAS 25

//estrutura com os dados a serem enviados
struct {
  uint16_t leme;
  uint16_t motor;
  byte servo;
  byte buzina;
  byte canhao;
} controle;

//estrutura com os dados recebidos
struct {
  byte bateria;
} barco;

CC1101Radio cc1101;

// ----------------------
// Contadores e Timers
// ----------------------
uint32_t msUltimoEnvio = 0;
#define msEntreEnvios 200

unsigned long previousMillis = 0;        // will store last time LED was updated
// constants won't change :
const long interval = 1000;           // interval at which to blink (milliseconds)


/**Função para converter os valores de tensão em umidade*/
float converte(float x, float in_min, float in_max, float out_min,
               float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

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
  controle.leme = 90;
  controle.motor = 0;
  controle.servo = 90;
  controle.buzina = false;
  controle.canhao = false;

  Serial.begin(9600);

  pinMode(3, INPUT);
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(6, INPUT);
  pinMode(7, OUTPUT);
  //inicializa o radio com os valores padrao
  cc1101.init();

  //define o endereco local do radio
  cc1101.deviceData.deviceAddress = CONTROLE;

  //define o endereço remoto
  cc1101.deviceData.remoteDeviceAddress = BARCO;

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


}

void loop() {
  char i = 0;
  // Verifica se o radio recebeu um pacote
  if (!pacoteRecebido()) {
    // Se não recebeu um pacote, verifica se está na hora de enviar os dados
    if (millis() > msUltimoEnvio + msEntreEnvios) {
      enviaDados();
      msUltimoEnvio = millis();
    }
  } else {
    if (barco.bateria) {
      digitalWrite(7, HIGH);
    }
    else {
      digitalWrite(7, LOW);
    }
  }
  //LE DADOS

  for ( i = 0 ; i < AMOSTRAS; i++) {
    controle.leme += analogRead(A0);
    delayMicroseconds(10);
  }
  controle.leme = controle.leme / AMOSTRAS;

  for ( i = 0 ; i < AMOSTRAS; i++) {
    controle.motor += analogRead(A1);
    delayMicroseconds(10);
  }
  controle.motor = controle.motor / AMOSTRAS;

  controle.buzina = digitalRead(6);
  controle.canhao = digitalRead(5);

  if (digitalRead(3) == 1 && controle.servo < 180) {
    controle.servo += 1; // goes from 0 degrees to 180 degrees
    delay(30);// waits 15ms for the servo to reach the position

  }
  if (digitalRead(4) == 1 && controle.servo > 0) {
    controle.servo -= 1; // goes from 180 degrees to 0 degrees
    delay(30); // waits 15ms for the servo to reach the position
  }


  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    mostraDados();
  }

}

void mostraDados() {
  Serial.println("------------------------------------\nCONTROLE\n");

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

}
// ----------------------
// Send & Receive
// ----------------------

void enviaDados() {
  //tamanho do pacote que sera enviado
  uint8_t dataLength = 9;

  //cria pacote
  CC1101Radio::CCPACKET pkt;

  //define o tamanho
  pkt.length = dataLength;

  //monta o pacote
  pkt.data[0] = cc1101.deviceData.remoteDeviceAddress;
  pkt.data[1] = cc1101.deviceData.deviceAddress;
  pkt.data[2] = lowByte(controle.leme);
  pkt.data[3] = highByte(controle.leme);
  pkt.data[4] = lowByte(controle.motor);
  pkt.data[5] = highByte(controle.motor);
  pkt.data[6] = controle.servo;
  pkt.data[7] = controle.buzina;
  pkt.data[8] = controle.canhao;

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
      barco.bateria = pkt.data[2];
      Serial.println("------------------------------------\n");
      Serial.print("barco.bateria: ");
      Serial.println(barco.bateria);
    } // crc & len>0
  } // cc1101.readData

  //Liga novamente a interrupção para permitir novos dados
  attachIntr();

  return true;
}
