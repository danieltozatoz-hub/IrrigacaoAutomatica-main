#include <Arduino.h>
#include <SoftwareSerial.h>
#include "WiFiEsp.h"
#include <ArduinoJson.h>
#include "config.h"

SoftwareSerial SerialESP(2, 3); 
WiFiEspClient client; 

struct Estado {
  int umidade = 0;
  bool bombaLigada = false;
  bool modoAuto = true;
  int limiteSeco = 30;
  unsigned long ultimaLeitura = 0;
} estado;

bool ultimaBombaServidor = false;

void lerSensor();
void controlarBomba();
void comunicarComServidor();

void setup() {
  Serial.begin(9600);   
  SerialESP.begin(9600);  

  pinMode(PIN_VCC_SENSOR, OUTPUT);
  pinMode(PIN_RELE, OUTPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);
  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_VCC_SENSOR, LOW);
  digitalWrite(PIN_RELE, HIGH); 
  digitalWrite(PIN_LED_VERDE, HIGH);
  digitalWrite(PIN_LED_VERMELHO, LOW);

  WiFi.init(&SerialESP);
  
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("ERRO: Módulo ESP-01 não encontrado!");
    while (true); 
  }

  Serial.print("Conectando ao Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado ao Wi-Fi!");
}

void loop() {
  if (millis() - estado.ultimaLeitura > 3000) {
    estado.ultimaLeitura = millis();
    
    lerSensor();
    comunicarComServidor();
    controlarBomba();       
  }
}

void lerSensor() {
  digitalWrite(PIN_VCC_SENSOR, HIGH); 
  delay(10); 
  
  int valorBruto = analogRead(PIN_SENSOR);
  digitalWrite(PIN_VCC_SENSOR, LOW); 

  estado.umidade = map(valorBruto, 1023, 0, 0, 100);
  
  if (estado.umidade < 0) estado.umidade = 0;
  if (estado.umidade > 100) estado.umidade = 100;
}

void comunicarComServidor() {
  if (client.connect(SERVER_IP, SERVER_PORT)) {
    client.println("GET /api/status HTTP/1.1");
    client.print("Host: "); client.println(SERVER_IP);
    client.println("Connection: close");
    client.println();

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break; 
    }
    
    String response = client.readString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (!error) {
      bool bombaServidor = doc["bomba_ligada"];
      
      if (bombaServidor != ultimaBombaServidor) {
        estado.bombaLigada = bombaServidor;
        estado.modoAuto = false; 
        ultimaBombaServidor = bombaServidor;
        Serial.println("Comando manual recebido do servidor!");
      }
    }
    client.stop();
  }

  if (estado.modoAuto) {
    if (estado.umidade < estado.limiteSeco) {
      estado.bombaLigada = true;
    } else if (estado.umidade >= estado.limiteSeco + 10) { 
      estado.bombaLigada = false;
    }
    ultimaBombaServidor = estado.bombaLigada; 
  }

  if (client.connect(SERVER_IP, SERVER_PORT)) {
    JsonDocument docOut;
    docOut["umidade"] = estado.umidade;
    docOut["bomba_ligada"] = estado.bombaLigada;
    
    String payload;
    serializeJson(docOut, payload);

    client.println("POST /api/update HTTP/1.1");
    client.print("Host: "); client.println(SERVER_IP);
    client.println("Content-Type: application/json");
    client.print("Content-Length: "); client.println(payload.length());
    client.println("Connection: close");
    client.println();
    client.println(payload); 
    
    client.stop();
  }
}

void controlarBomba() {
  if (estado.bombaLigada) {
    digitalWrite(PIN_RELE, LOW);          
    digitalWrite(PIN_LED_VERMELHO, HIGH); 
    digitalWrite(PIN_LED_VERDE, LOW);             
  } else {
    digitalWrite(PIN_RELE, HIGH);        
    digitalWrite(PIN_LED_VERMELHO, LOW);  
    digitalWrite(PIN_LED_VERDE, HIGH);    
    noTone(PIN_BUZZER);                  
  }
}