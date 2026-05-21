// Pinos dos encoders
#define ENC1_A 22  // PA0
#define ENC1_B 23  // PA1
#define ENC2_A 24  // PA2
#define ENC2_B 25  // PA3

// LEDs (5 para cada encoder)
const int leds1[5] = {2, 3, 4, 5, 6};
const int leds2[5] = {8, 9, 10, 11, 12};

long pos1 = 0;
long pos2 = 0;

uint8_t lastState1 = 0;
uint8_t lastState2 = 0;

// ===== DIREÇÃO =====
int dir1 = -1;
int dir2 = -1;

// ===== CONFIG =====
float total_degrees = 720.0;
const float pulses_per_rev = 4096.0;

long MAX_POS;
const long MIN_POS = 0;

float gammaValue = 2.2;

// PWM útil
const int PWM_MIN = 0;
const int PWM_MAX = 255;

// controle de tempo
unsigned long lastPWM = 0;
unsigned long lastSerial = 0;

void updateBargraph(float norm, const int *leds) {

  norm = constrain(norm, 0.0, 1.0);

  for (int i = 0; i < 5; i++) {

    float start = i * 0.2;
    float end   = start + 0.2;

    float level;

    if (norm >= end) {
      level = 1.0;
    } 
    else if (norm <= start) {
      level = 0.0;
    } 
    else {
      level = (norm - start) / 0.2;
    }

    level = pow(level, gammaValue);

    int pwm = PWM_MIN + level * (PWM_MAX - PWM_MIN);
    pwm = constrain(pwm, 0, 255);

    analogWrite(leds[i], pwm);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(ENC1_A, INPUT_PULLUP);
  pinMode(ENC1_B, INPUT_PULLUP);
  pinMode(ENC2_A, INPUT_PULLUP);
  pinMode(ENC2_B, INPUT_PULLUP);

  // LEDs
  for (int i = 0; i < 5; i++) {
    pinMode(leds1[i], OUTPUT);
    pinMode(leds2[i], OUTPUT);
  }

  uint8_t porta = PINA;
  lastState1 = ((porta >> 0) & 1) | (((porta >> 1) & 1) << 1);
  lastState2 = ((porta >> 2) & 1) | (((porta >> 3) & 1) << 1);

  float pulses_per_degree = pulses_per_rev / 360.0;
  MAX_POS = total_degrees * pulses_per_degree;
}

void loop() {

  // ===== LEITURA RÁPIDA =====
  uint8_t porta = PINA;

  uint8_t state1 = ((porta >> 0) & 1) | (((porta >> 1) & 1) << 1);
  uint8_t state2 = ((porta >> 2) & 1) | (((porta >> 3) & 1) << 1);

  static const int8_t table[4][4] = {
    { 0, -1,  1,  0},
    { 1,  0,  0, -1},
    {-1,  0,  0,  1},
    { 0,  1, -1,  0}
  };

  pos1 += dir1 * table[lastState1][state1];
  pos2 += dir2 * table[lastState2][state2];

  lastState1 = state1;
  lastState2 = state2;

  // saturação
  if (pos1 < MIN_POS) pos1 = MIN_POS;
  if (pos1 > MAX_POS) pos1 = MAX_POS;

  if (pos2 < MIN_POS) pos2 = MIN_POS;
  if (pos2 > MAX_POS) pos2 = MAX_POS;

  if (millis() - lastPWM >= 2) {
    lastPWM = millis();

    float norm1 = (float)pos1 / MAX_POS;
    float norm2 = (float)pos2 / MAX_POS;

    updateBargraph(norm1, leds1);
    updateBargraph(norm2, leds2);
  }

  // ===== SERIAL INPUT =====
  if (Serial.available() > 0) {

    String input = Serial.readStringUntil('\n');

    if (input.length() == 0) return;

    int target = input.toInt();
    target = constrain(target, 0, 100);

    float normTarget = target / 100.0;
    int pwmTarget = PWM_MIN + normTarget * (PWM_MAX - PWM_MIN);

    float norm1 = (float)pos1 / MAX_POS;
    norm1 = pow(norm1, gammaValue);
    int currentPWM1 = PWM_MIN + norm1 * (PWM_MAX - PWM_MIN);

    float norm2 = (float)pos2 / MAX_POS;
    norm2 = pow(norm2, gammaValue);
    int currentPWM2 = PWM_MIN + norm2 * (PWM_MAX - PWM_MIN);

    int deltaPWM1 = pwmTarget - currentPWM1;
    int deltaPWM2 = pwmTarget - currentPWM2;

    float deltaNorm1 = (float)deltaPWM1 / (PWM_MAX - PWM_MIN);
    float deltaNorm2 = (float)deltaPWM2 / (PWM_MAX - PWM_MIN);

    pos1 += deltaNorm1 * MAX_POS;
    pos2 += deltaNorm2 * MAX_POS;

    if (pos1 < MIN_POS) pos1 = MIN_POS;
    if (pos1 > MAX_POS) pos1 = MAX_POS;

    if (pos2 < MIN_POS) pos2 = MIN_POS;
    if (pos2 > MAX_POS) pos2 = MAX_POS;

    Serial.print("Ajuste para: ");
    Serial.print(target);
    Serial.println("%");
  }

  // ===== DEBUG =====
  if (millis() - lastSerial >= 100) {
    lastSerial = millis();

    Serial.print("Pos1: ");
    Serial.print(pos1);
    Serial.print(" | Pos2: ");
    Serial.print(pos2);
    Serial.println();
  }
}
