#include <SPI.h>

// ===================================================
// ADS131M08 SETTINGS
// ===================================================

#define INPUT_MODE_DIFFERENTIAL true
#define SAMPLE_RATE_HZ 250
#define SPI_SPEED 1000000

#define CS_PIN     10
#define DRDY_PIN   2
#define RESET_PIN  9

volatile bool dataReady = false;

// ===================================================

struct SamplePacket
{
  uint8_t sync1;
  uint8_t sync2;

  uint32_t counter;

  int32_t ch[8];
};

uint32_t sampleCounter = 0;

// ===================================================

void drdyISR()
{
  dataReady = true;
}

// ===================================================

void writeRegister(uint8_t reg, uint8_t value)
{
  digitalWrite(CS_PIN, LOW);

  SPI.transfer(0x40 | reg);
  SPI.transfer(0x00);
  SPI.transfer(value);

  digitalWrite(CS_PIN, HIGH);
}

// ===================================================

void sendCommand(uint8_t cmd)
{
  digitalWrite(CS_PIN, LOW);
  SPI.transfer(cmd);
  digitalWrite(CS_PIN, HIGH);
}

// ===================================================

int32_t read24()
{
  int32_t value = 0;

  value |= ((uint32_t)SPI.transfer(0x00)) << 16;
  value |= ((uint32_t)SPI.transfer(0x00)) << 8;
  value |= ((uint32_t)SPI.transfer(0x00));

  if (value & 0x800000)
    value |= 0xFF000000;

  return value;
}

// ===================================================

void setDataRate(uint16_t hz)
{
  uint8_t dr;

  switch (hz)
  {
    case 125:
      dr = 0x00;
      break;

    case 250:
      dr = 0x04;
      break;

    case 500:
      dr = 0x05;
      break;

    default:
      dr = 0x04;
      break;
  }

  writeRegister(0x03, dr);
}

// ===================================================

void readAllChannels(int32_t *raw)
{
  // STATUS word
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(0x00);

  for (int i = 0; i < 8; i++)
  {
    raw[i] = read24();
  }
}

// ===================================================

void setup()
{
  Serial.begin(500000);

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  pinMode(DRDY_PIN, INPUT_PULLUP);

  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);

  attachInterrupt(
      digitalPinToInterrupt(DRDY_PIN),
      drdyISR,
      FALLING);

  SPI.begin();

  SPI.beginTransaction(
      SPISettings(
          SPI_SPEED,
          MSBFIRST,
          SPI_MODE1));

  // ADC reset
  digitalWrite(RESET_PIN, LOW);
  delay(10);

  digitalWrite(RESET_PIN, HIGH);
  delay(100);

  uint8_t chConfig = 0x07;

  if (!INPUT_MODE_DIFFERENTIAL)
    chConfig |= 0x01;

  for (uint8_t ch = 0; ch < 8; ch++)
  {
    writeRegister(0x05 + ch, chConfig);
  }

  setDataRate(SAMPLE_RATE_HZ);

  sendCommand(0x08); // START

  delay(100);
}

// ===================================================

void loop()
{
  if (!dataReady)
    return;

  dataReady = false;

  int32_t raw[8];

  digitalWrite(CS_PIN, LOW);

  readAllChannels(raw);

  digitalWrite(CS_PIN, HIGH);

  SamplePacket pkt;

  pkt.sync1 = 0xAA;
  pkt.sync2 = 0x55;

  pkt.counter = sampleCounter++;

  for (int i = 0; i < 8; i++)
  {
    pkt.ch[i] = raw[i];
  }

  Serial.write(
      (uint8_t *)&pkt,
      sizeof(pkt));
}