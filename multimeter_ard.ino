#include <math.h>

#define N_MODES 5
#define ARD_VOLT 5.10f
#define CAP_PIN 8

#define CAP_CHARGE_MAX .5f
#define PI 3.1415926535

volatile int mode = 0;

volatile bool charging_cap = false;
volatile unsigned long cap_t_init = 0;
volatile unsigned long cap_t_final = 0;

char modes[][64] = { "Voltage", "Resistance", "Capacitance", "Inductance", "Diode" };
char units[][8] = { "V", "kOhm", "uF", "mH", "V" };

void cycleMode() {
  mode++;
  mode %= N_MODES;
}

void startRead() {
  if (mode == 2) {
    charging_cap = true;
    cap_t_init = millis();
    digitalWrite(CAP_PIN, HIGH);
  }
}

void stopRead() {
  charging_cap = false;
  cap_t_final = millis();

  digitalWrite(CAP_PIN, LOW);
}

void setup() {
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(CAP_PIN, OUTPUT);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

  attachInterrupt(digitalPinToInterrupt(2), cycleMode, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), startRead, FALLING);

  ACSR = 0;
  ADCSRB = 0;

  Serial.begin(115200);
}

void loop() {
  Serial.print("MODE : ");
  Serial.print(modes[mode]);
  Serial.println();

  float result;

  if (mode == 0) {
    float MAX_VOLT = 32.00f;

    float volt = analogRead(A0) / 1023.0;
    result = volt * MAX_VOLT;
  } else if (mode == 1) {
    float rs = 19.16f;
    float res_vol = analogRead(A0) / 1023.0;

    result = rs * (1 / res_vol - 1);
  }
  if (mode == 2) {
    float r = 97.7f;
    float charge_vol = analogRead(A1) / 1023.0;

    if (charging_cap) {
      Serial.print("Charging ");
      Serial.print(charge_vol);
      Serial.println();
    }
    if (charge_vol >= CAP_CHARGE_MAX) stopRead();

    unsigned long dt = cap_t_final - cap_t_init;
    result = dt / (r * log(1 / (1.0 - CAP_CHARGE_MAX)));
  }
  if (mode == 3) {
    unsigned long timeout = 1e6;
    unsigned long t = 0;
    unsigned long total;

    PORTB |= 1;
    _delay_ms(50);
    PORTB &= 0;

    //next three while loops wait for full wave to start and finish
    //aco is a bit within ACSR register that represents output of the comparator

    while ((ACSR & (1<<ACO)) && t < timeout) { t++; }

    unsigned long start = micros();
    while (!(ACSR & (1<<ACO)) && t < timeout) { t++; }
    while ((ACSR & (1<<ACO)) && t < timeout) { t++; }
    total = micros() - start - 4;

    double C = 1.046;
    double T = total;

    if (t >= timeout) {
      result = 0;
    } else {
      result = (T*T) / (4.0*PI*PI*C) / 1e3;
      delay(100);
    }
  } else if (mode == 4) {
    PORTB |= 1;
    result = analogRead(A2) / 1023.0 * 5;
  }

  Serial.print(result);
  Serial.print(units[mode]);
  Serial.println();
  delay(50);
}
