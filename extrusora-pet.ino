#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <max6675.h>

// Endereço I2C 0x27 e display 16x2. Pinos padrão no ESP32: SDA = 21, SCL = 22
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Configurações do MAX6675 (Pinos atualizados para ESP32) ---
int pinoSO = 19;
int pinoCS = 5;
int pinoSCK = 18;
MAX6675 termopar(pinoSCK, pinoCS, pinoSO);

// --- Configurações do Motor (Pinos atualizados para ESP32) ---
const int pinoSinal = 4;
const int pinoSleep = 27;  // Pino SLEEP do driver (nível alto liga)
const int passosPorVolta = 200;
const int microstepping = 32;
const float pulsosPorVoltaMotor = passosPorVolta * microstepping;  // 6400 pulsos/volta do motor

// --- CAIXA DE REDUÇÃO ---
const float REDUCAO_TOTAL = 56.25;  // Relação de redução total

volatile unsigned long contadorPulsos = 0;
unsigned long tempoAnteriorRPM = 0;
const int intervaloMedicao = 1000;  

// --- Pinos e Limites (Pino do Relé atualizado para ESP32) ---
const int pinoRele = 26;
const float LIMITE_ALTO = 245.0;
const float LIMITE_BAIXO = 235.0;
bool aquecendo = false;

// --- Limites para Sleep do Motor ---
const float LIMITE_SLEEP_TEMP = 225.0;     // Temperatura mínima para ligar
const float LIMITE_SLEEP_RPM = 0.13;       // RPM mínimo para ligar (carretel)
bool motorSleep = true;                    // Começa desligado (sleep)

// ESP32 EXIGE O IRAM_ATTR PARA FUNÇÕES DE INTERRUPÇÃO
void IRAM_ATTR contarPulso() {
  contadorPulsos++;
}

void setup() {
  lcd.init();
  lcd.backlight();
  Serial.begin(115200); // Velocidade padrão recomendada para ESP32 (ajuste no monitor serial)
  
  pinMode(pinoRele, OUTPUT);
  digitalWrite(pinoRele, LOW);
  
  // Configura o pino SLEEP
  pinMode(pinoSleep, OUTPUT);
  digitalWrite(pinoSleep, LOW);  // Começa com motor desligado (LOW = sleep)
  
  pinMode(pinoSinal, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinoSinal), contarPulso, RISING);
  
  delay(500);
  
  lcd.setCursor(0, 0);
  lcd.print("Extrusora PET");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(2000);
  lcd.clear();
  
  Serial.println("=== EXTRUSORA PET (ESP32) ===");
  Serial.println("Motor liga quando:");
  Serial.println("- Temp >= 225C");
  Serial.println("- RPM >= 0.13");
  Serial.println("=============================");
}

void loop() {
  // --- LEITURA DA TEMPERATURA ---
  float temperaturaC = termopar.readCelsius();
  
  // Verifica se o termopar está desconectado
  if (isnan(temperaturaC)) {
    lcd.setCursor(0, 0);
    lcd.print("ERRO: Termopar! ");
    delay(1000);
    return;
  }
  
  // --- CÁLCULO DO RPM ---
  unsigned long tempoAtual = millis();
  static float rpmAtual = 0;
  
  if (tempoAtual - tempoAnteriorRPM >= intervaloMedicao) {
    noInterrupts(); 
    unsigned long pulsosNoIntervalo = contadorPulsos;
    contadorPulsos = 0;
    interrupts();
    
    // Calcula RPM do MOTOR primeiro
    float frequenciaHz = pulsosNoIntervalo / (intervaloMedicao / 1000.0);
    float rpmMotor = (frequenciaHz * 60.0) / pulsosPorVoltaMotor;
    
    // Aplica a redução para obter RPM do CARRETEL
    rpmAtual = rpmMotor / REDUCAO_TOTAL;
    
    // Limita para valores realistas
    if (rpmAtual > 10) rpmAtual = 0;
    if (rpmAtual < 0) rpmAtual = 0;
    
    tempoAnteriorRPM = tempoAtual;
    
    // Debug opcional
    Serial.print("Pulsos: ");
    Serial.print(pulsosNoIntervalo);
    Serial.print(" | RPM Carretel: ");
    Serial.print(rpmAtual, 3);
    Serial.print(" | Motor Sleep: ");
    Serial.println(motorSleep ? "SIM" : "NAO");
  }
  
  // --- CONTROLE DO SLEEP DO MOTOR ---
  bool condicaoTemp = (temperaturaC >= LIMITE_SLEEP_TEMP);
  bool condicaoRPM = (rpmAtual >= LIMITE_SLEEP_RPM);
  
  if (condicaoTemp && condicaoRPM && motorSleep) {
    digitalWrite(pinoSleep, HIGH);
    motorSleep = false;
    Serial.print("MOTOR: LIGADO (temp=");
    Serial.print(temperaturaC, 1);
    Serial.print("C, rpm=");
    Serial.print(rpmAtual, 3);
    Serial.println(")");
  }
  else if ((!condicaoTemp || !condicaoRPM) && !motorSleep) {
    digitalWrite(pinoSleep, LOW);
    motorSleep = true;
    Serial.print("MOTOR: DESLIGADO (temp=");
    Serial.print(temperaturaC, 1);
    Serial.print("C, rpm=");
    Serial.print(rpmAtual, 3);
    Serial.println(")");
  }
  
  // --- CONTROLE DO RELÉ ---
  String statusRele = "";
  
  if (temperaturaC >= LIMITE_ALTO) {
    digitalWrite(pinoRele, LOW);
    aquecendo = false;
    statusRele = "[OFF]";
  } 
  else if (temperaturaC <= LIMITE_BAIXO) {
    digitalWrite(pinoRele, HIGH);
    aquecendo = true;
    statusRele = "[ON] ";
  }
  else {
    statusRele = aquecendo ? "[ON] " : "[OFF]";
  }
  
  // --- DISPLAY LCD ---
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperaturaC, 1);
  lcd.print((char)223);
  lcd.print("C ");
  
  lcd.setCursor(10, 0);
  lcd.print(statusRele);
  
  lcd.setCursor(0, 1);
  lcd.print("RPM:");
  
  if (motorSleep) {
    lcd.print(" -- ");
  } else {
    lcd.print(rpmAtual, 3);
  }
  lcd.print("     ");
  
  lcd.setCursor(15, 0);
  if (motorSleep) {
    lcd.print("Z");
  } else {
    lcd.print("R");
  }
  
  delay(500);
}