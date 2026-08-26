//ADS131m08 EEG/ECG module
//MC: Arduino UNO
//All 8 channels working
//Baud Rate: 500000


#include <SPI.h>
#include <Arduino.h>

// ---------------- USER SETTINGS ---------------- //
#define INPUT_MODE_DIFFERENTIAL true // true=differential, false=single-ended
#define SAMPLE_RATE_HZ 250            // ADC sampling rate
#define SPI_SPEED 1000000            // SPI clock in Hz
#define VREF 2.5                     // ADC reference voltage (V)
#define PGA 24                       // Programmable gain
// ------------------------------------------------ //

#define CS_PIN 10
#define DRDY_PIN 2
#define RESET_PIN 9

volatile bool dataReady = false;
#define MAX24 8388607 // 2^23 -1 for 24-bit

// -------------------- DRDY ISR -------------------- //
void drdyISR() { dataReady = true; }

// -------------------- SPI Helpers -------------------- //
void writeRegister(uint8_t reg, uint8_t value) {
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(0x40 | reg); // WREG command
  SPI.transfer(0x00);       // Write 1 register
  SPI.transfer(value);
  digitalWrite(CS_PIN, HIGH);
}

void sendCommand(uint8_t cmd) {
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(cmd);
  digitalWrite(CS_PIN, HIGH);
}

long read24() {
  long value = 0;
  for (int i = 0; i < 3; i++) {
    value <<= 8;
    value |= SPI.transfer(0x00);
  }
  if (value & 0x800000) value |= 0xFF000000;
  return value;
}

// -------------------- ADC Conversion -------------------- //
long adcToMicrovoltInt(long adc) {
  return (adc * (long)(VREF * 1e6 / PGA)) / MAX24;
}

// -------------------- Data Rate -------------------- //
void setDataRate(uint16_t hz) {
  uint8_t dr = 0;
  switch (hz) {
    case 125: dr = 0x0; break;
    case 250: dr = 0x4; break;
    case 500: dr = 0x5; break;
    default: dr = 0x4;  // default 250 Hz
  }
  writeRegister(0x03, dr); // CONFIG1
}

// -------------------- Setup -------------------- //
void setup() {
  Serial.begin(500000);

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  pinMode(DRDY_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(DRDY_PIN), drdyISR, FALLING);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);

  SPI.begin();
  SPI.beginTransaction(SPISettings(SPI_SPEED, MSBFIRST, SPI_MODE1));

  // Reset ADC
  digitalWrite(RESET_PIN, LOW);
  delay(10);
  digitalWrite(RESET_PIN, HIGH);
  delay(10);

  // Configure channels
  uint8_t chConfig = 0x07; // PGA=24
  if (INPUT_MODE_DIFFERENTIAL)
    chConfig |= 0x00;
  else
    chConfig |= 0x01;

  for (uint8_t ch = 0; ch < 8; ch++) {
    writeRegister(0x05 + ch, chConfig);
  }

  setDataRate(SAMPLE_RATE_HZ);

  sendCommand(0x08); // START
}

// -------------------- Read All Channels -------------------- //
void readAllChannels(long* raw, long* microV) {
  // Discard STATUS bytes (3 bytes)
  for (int i = 0; i < 3; i++) {
    SPI.transfer(0x00);
  }

  // Read 8 channels (3 bytes each)
  for (int i = 0; i < 8; i++) {
    raw[i] = read24();
    microV[i] = adcToMicrovoltInt(raw[i]);
  }
}

// -------------------- Main Loop -------------------- //
void loop() {
  while (dataReady) {
    dataReady = false;

    digitalWrite(CS_PIN, LOW);

    long raw[8];
    long microV[8];
    readAllChannels(raw, microV);

    digitalWrite(CS_PIN, HIGH);

    // Send 8 channels (no timestamp)
    for (int i = 0; i < 8; i++) {
      Serial.print(microV[i]);
      if (i < 7) Serial.print(",");
    }
    Serial.println();
  }
}
