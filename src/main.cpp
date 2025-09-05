#include <WiFi.h>
#include <ESPUI.h>
#include <TM1637Display.h>  // Librería para TM1637

// Credenciales Wi-Fi Wokwi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Pines para TM1637 (un solo módulo para ambos scores)
#define CLK 2
#define DIO 15
TM1637Display display (CLK, DIO);

// Pines para sensores HC-SR04
#define TRIG_A 26
#define ECHO_A 25
#define TRIG_B 5
#define ECHO_B 4

// Boton Reset
#define RESET  34

// Variables
volatile int golesA = 0;
volatile int golesB = 0;
int lblTanteador;  // ID para label en ESPUI
unsigned long ultimoGolA = 0;
unsigned long ultimoGolB = 0;
const long tiempoEspera = 1000;  // 1 segundo de espera entre goles
const float distanciaGol = 10.0;  // Distancia en cm para detectar gol

// Segmento personalizado para guion "-"
const uint8_t SEG_DASH = 0x40;  // Solo segmento g encendido

// Función para medir distancia con HC-SR04
float obtenerDistancia (int trigPin, int echoPin) {
  digitalWrite (trigPin, LOW);
  delayMicroseconds (2);
  digitalWrite (trigPin, HIGH);
  delayMicroseconds (10);
  digitalWrite (trigPin, LOW);
  
  long duration = pulseIn (echoPin, HIGH, 30000);  // Timeout de 30ms
  if (duration == 0) return 100.0;  // Valor alto si no hay eco
  float distance = duration * 0.034 / 2;  // Distancia en cm
  return distance;
}

// Actualizar display TM1637
void actualizarTanteador () {
  uint8_t segs [4];
  segs [0] = display.encodeDigit (golesA); // Dígito para jugador A
  segs [1] = SEG_DASH;                     // Guion "-"
  segs [2] = display.encodeDigit (golesB); // Dígito para jugador B
  segs [3] = 0x00;                         // Blanco
  display.setSegments (segs);
  display.
}

// Callback para reset en UI
void resetCallback (Control *sender, int value) {
  golesA = 0;
  golesB = 0;
  actualizarTanteador ();
  ESPUI.updateLabel (lblTanteador, "Marcador: A 0 - B 0");
}

void setup() {
  Serial.begin (115200);

  // Configurar TM1637
  display.setBrightness (0x0f);  // Brillo máximo (0x00 a 0x0f)

  // Configurar pines sensores y reset
  pinMode (TRIG_A, OUTPUT);
  pinMode (ECHO_A, INPUT);
  pinMode (TRIG_B, OUTPUT);
  pinMode (ECHO_B, INPUT);
  pinMode (RESET, INPUT_PULLUP);

  // Inicializar display
  actualizarTanteador ();

  // Conectar WiFi
  WiFi.begin (ssid, password);

  Serial.print ("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay (500);
    Serial.print (".");
  }

  Serial.print (" WiFi conectado, IP: ");
  Serial.println (WiFi.localIP());

  // Configurar ESPUI
  ESPUI.label ("Metegol Control", ControlColor::Carrot, "Bienvenido al controlador de metegol");
  lblTanteador = ESPUI.label ("Marcador", ControlColor::Emerald, "A 0 - B 0");
  ESPUI.button ("Resetear Marcador", &resetCallback, ControlColor::Peterriver);
  ESPUI.begin ("Metegol ESP32");
}

void loop() {
  unsigned long currentTime = millis();

  // Leer arco jugador A
  float distanceA = obtenerDistancia (TRIG_A, ECHO_A);
  if (distanceA < distanciaGol && (currentTime - ultimoGolA) > tiempoEspera) {
    golesA = (golesA < 9) ? golesA + 1 : 0;  // Máx 9, resetea a 0 si pasa
    ultimoGolA = currentTime;
    actualizarTanteador ();
    ESPUI.updateLabel (lblTanteador, String ("Marcador: A " + String (golesA) + " - B " + String (golesB)));
  }

  // Leer arco jugador B
  float distanceB = obtenerDistancia (TRIG_B, ECHO_B);
  if (distanceB < distanciaGol && (currentTime - ultimoGolB) > tiempoEspera) {
    golesB = (golesB < 9) ? golesB + 1 : 0;
    ultimoGolB = currentTime;
    actualizarTanteador();
    ESPUI.updateLabel (lblTanteador, String ("Marcador: A " + String (golesA) + " - B " + String (golesB)));
  }

  // Chequear reset físico (opcional)
  if (!digitalRead (RESET)) {
    golesA = 0;
    golesB = 0;
    actualizarTanteador ();
    ESPUI.updateLabel (lblTanteador, "Marcador: A 0 - B 0");
    delay(200);  // antirrebote
  }

  delay(100);  // Leer cada 100ms para no saturar
}