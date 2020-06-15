#include <WioLTEforArduino.h>

#define APN               "sakura"
#define USERNAME          ""
#define PASSWORD          ""
#define SENSOR_PIN        WIOLTE_D38
#define MEASUREMENT       "productA"
#define HARDWARE          "wiolte"
#define INFLUXDB_URL      "http://%%IPアドレス%%:8086/api/v2/write?bucket=testdb/autogen"
#define INTERVAL          60000

WioLTE Wio;
char iccid[20];

void setup() {

  delay(1000);

  Wio.LedSetRGB(100, 0, 0);

  SerialUSB.println("");
  SerialUSB.println("--- START ---------------------------------------------------");

  SerialUSB.println("### I/O Initialize.");
  Wio.Init();

  SerialUSB.println("### Power supply ON.");
  Wio.PowerSupplyLTE(true);
  delay(500);

  SerialUSB.println("### Turn on or reset.");
  if (!Wio.TurnOnOrReset()) {
    SerialUSB.println("## ERROR! ##");
    return;
  } else {
    SerialUSB.println("## OK! ##");
  }

  SerialUSB.print("### GetICCID.");
  if (!Wio.GetICCID(iccid, sizeof(iccid))) {
    SerialUSB.println("## ERROR! ##");
    return;
  } else {
    SerialUSB.println("## OK! ##");
    SerialUSB.print("ICCID:");
    SerialUSB.println(iccid);
  }

  SerialUSB.println("### Connecting to \""APN"\".");
  if (!Wio.Activate(APN, USERNAME, PASSWORD)) {
    SerialUSB.println("## ERROR! ##");
    return;
  } else {
    SerialUSB.println("## OK! ##");
  }

  TemperatureAndHumidityBegin(SENSOR_PIN);

  SerialUSB.println("### Setup completed.");
  SerialUSB.println("");
  Wio.LedSetRGB(0, 100, 0);

err:
  delay(2000);
}


void loop() {

  SerialUSB.println("### Get Sensor data.");
  float temp;
  float humi;
  if (!TemperatureAndHumidityRead(&temp, &humi)) {
    SerialUSB.println("## ERROR! ##");
    goto err;
  } else {
    SerialUSB.println("## OK! ##");
    SerialUSB.print("temperature = ");
    SerialUSB.print(temp);
    SerialUSB.println("C");
    SerialUSB.print("humidity = ");
    SerialUSB.print(humi);
    SerialUSB.println("%");
    SerialUSB.println("");
  }

  SerialUSB.println("### Post.");
  char data[200];
  int status;
  sprintf(data, "%s,hardware=%s,iccid=%s temperature=%f,humidity=%f" , MEASUREMENT , HARDWARE , iccid , temp , humi);
  SerialUSB.print("Post:");
  SerialUSB.print(data);
  SerialUSB.println("");
  if (!Wio.HttpPost(INFLUXDB_URL, data, &status)) {
    SerialUSB.println("## ERROR! ##");
    goto err;
  } else {
    SerialUSB.println("## OK! ##");
  }
  SerialUSB.print("Status:");
  SerialUSB.println(status);

err:
  SerialUSB.println("### Wait.");
  delay(INTERVAL);
}

// From grove-temperature-and-humidity-sensor.ino

int TemperatureAndHumidityPin;

void TemperatureAndHumidityBegin(int pin)
{
  TemperatureAndHumidityPin = pin;
  DHT11Init(TemperatureAndHumidityPin);
}

bool TemperatureAndHumidityRead(float* temperature, float* humidity)
{
  byte data[5];

  DHT11Start(TemperatureAndHumidityPin);
  for (int i = 0; i < 5; i++) data[i] = DHT11ReadByte(TemperatureAndHumidityPin);
  DHT11Finish(TemperatureAndHumidityPin);

  if (!DHT11Check(data, sizeof (data))) return false;
  if (data[1] >= 10) return false;
  if (data[3] >= 10) return false;

  *humidity = (float)data[0] + (float)data[1] / 10.0f;
  *temperature = (float)data[2] + (float)data[3] / 10.0f;

  return true;
}

void DHT11Init(int pin)
{
  digitalWrite(pin, HIGH);
  pinMode(pin, OUTPUT);
}

void DHT11Start(int pin)
{
  // Host the start of signal
  digitalWrite(pin, LOW);
  delay(18);

  // Pulled up to wait for
  pinMode(pin, INPUT);
  while (!digitalRead(pin)) ;

  // Response signal
  while (digitalRead(pin)) ;

  // Pulled ready to output
  while (!digitalRead(pin)) ;
}

byte DHT11ReadByte(int pin)
{
  byte data = 0;

  for (int i = 0; i < 8; i++) {
    while (digitalRead(pin)) ;

    while (!digitalRead(pin)) ;
    unsigned long start = micros();

    while (digitalRead(pin)) ;
    unsigned long finish = micros();

    if ((unsigned long)(finish - start) > 50) data |= 1 << (7 - i);
  }

  return data;
}

void DHT11Finish(int pin)
{
  // Releases the bus
  while (!digitalRead(pin)) ;
  digitalWrite(pin, HIGH);
  pinMode(pin, OUTPUT);
}

bool DHT11Check(const byte* data, int dataSize)
{
  if (dataSize != 5) return false;

  byte sum = 0;
  for (int i = 0; i < dataSize - 1; i++) {
    sum += data[i];
  }

  return data[dataSize - 1] == sum;
}