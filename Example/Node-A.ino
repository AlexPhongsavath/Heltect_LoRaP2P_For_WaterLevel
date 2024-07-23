#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "HT_SSD1306Wire.h"
#include <esp_sleep.h>
#include <Ultrasonic.h>

#define RF_FREQUENCY 433000000 // Hz   433 for Asia

#define TX_OUTPUT_POWER 19 // dBm

#define LORA_BANDWIDTH 0         // [0: 125 kHz, \
                                 //  1: 250 kHz, \
                                 //  2: 500 kHz, \
                                 //  3: Reserved]
#define LORA_SPREADING_FACTOR 12 // [SF7..SF12]
#define LORA_CODINGRATE 4        // [1: 4/5, \
                                 //  2: 4/6, \
                                 //  3: 4/7, \
                                 //  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8   // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0    // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

#define RX_TIMEOUT_VALUE 1000
#define BUFFER_SIZE 30 // Define the payload size here
#define uS_TO_S_FACTOR 1000000

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

int16_t rssi, rxSize;

bool lora_idle = true;

static RadioEvents_t RadioEvents;
void OnTxDone(void);
void OnTxTimeout(void);

extern SSD1306Wire display;

String hexString;
unsigned long receiving = 0;
unsigned long sending = 0;

Ultrasonic ultrasonic(12, 13);
int distance;
int level;

void setup()
{
  Serial.begin(115200);
  Mcu.begin();
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  display.drawString(0, 0, "Heltec.LoRa Initial success!");
  display.display();
  delay(1000);
  display.clear();

  txNumber = 0;

  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  RadioEvents.RxDone = OnRxDone;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);
}

void sendData()
{

  distance = ultrasonic.read();
  level = map(distance, 0, 106, 220, 106);

  if (distance > 106)
  {
    distance = 106;
  }

  else if (level < 110)
  {
    level = 0;
  }

  String waterLevel = String((level * 10), HEX).c_str();

  if (waterLevel.length() == 1)
  {
    waterLevel = "000" + waterLevel;
  }
  else if (waterLevel.length() == 2)
  {
    waterLevel = "00" + waterLevel;
  }
  else if (waterLevel.length() == 3)
  {
    waterLevel = "0" + waterLevel;
  }

  hexString = waterLevel;

  // Calculate the length of the byte array
  size_t byteArrayLength = hexString.length() / 2;
  byte byteArray[byteArrayLength + 1];

  // Convert the hex string to a byte array
  for (size_t i = 0; i < byteArrayLength; ++i)
  {
    // Extract two characters from the string
    String byteString = hexString.substring(i * 2, i * 2 + 2);
    // Convert the extracted characters to a byte
    byteArray[i] = (byte)strtol(byteString.c_str(), NULL, 16);
  }

  // Calculate the checksum
  byte checksum = calculateChecksum8(byteArray, byteArrayLength);
  byteArray[byteArrayLength] = checksum;
  String Checksum = String(checksum, HEX).c_str();

  if (hexString.length() == 4)
  {
    // Print the byte array
    Serial.print("Byte Array: ");
    for (size_t i = 0; i <= byteArrayLength; ++i)
    {
      Serial.print("0x");
      if (byteArray[i] < 0x10)
      {
        Serial.print("0");
      }
      Serial.print(byteArray[i], HEX);
      if (i <= byteArrayLength - 1)
      {
        Serial.print(", ");
      }
    }
  }

  if (lora_idle == true)
  {
    delay(1000);

    sending = 0;
    receiving = millis();
    String send_time = String(sending) + "ms";

    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    // Manage Payload
    String data = "01" + waterLevel + Checksum;

    // Print Data to OLED
    display.drawString(0, 0, "Sending: ");
    display.drawString(55, 0, String(data));
    String result = String(((float)level / 100), 2) + " m";
    display.drawString(0, 10, "Water level: ");
    display.drawString(70, 10, result);
    display.drawString(0, 20, "Sending time: ");
    display.drawString(75, 20, send_time);
    display.display();
    delay(2000);
    display.clear();

    // start a package
    sprintf(txpacket, "%s", hexString); 

    if (strlen(txpacket) == 4)
    {
      Serial.printf("\r\nsending packet \"%s\" , length %d\r\n", txpacket, strlen(txpacket));

      // send the package out
      Radio.Send((uint8_t *)txpacket, strlen(txpacket)); 
      Radio.Rx(0);
    }
  }

  Radio.IrqProcess();
}

void loop()
{

  sendData();
}

byte calculateChecksum8(const byte *data, size_t length)
{
  uint16_t sum = 0;

  // Sum all bytes
  for (size_t i = 0; i < length; ++i)
  {
    sum += data[i];
  }

  // Limit sum to 8 bits (equivalent to sum % 256)
  sum = sum & 0xFF;

  // Calculate 2's complement
  byte checksum = ~sum + 1;

  return checksum;
}

void OnTxDone(void)
{
  Serial.println("TX done......");
  // lora_idle = true;
}

void OnTxTimeout(void)
{
  Radio.Sleep();
  Serial.println("TX Timeout......");
  // lora_idle = true;
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
  rssi = rssi;
  rxSize = size;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n", rxpacket, rssi, rxSize);

  // Print Handcheck When node A receive message from node B
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 20, "Hand Check!");

  display.drawString(0, 30, "Entering deep sleep mode");

  sending = millis() - receiving;
  String received_time = String(sending) + "ms";
  display.drawString(0, 40, "Receive time: ");
  display.drawString(75, 40, received_time);
  display.display();

  delay(5000);

  // Enter deep sleep for 1 minute (60 seconds)
  esp_sleep_enable_timer_wakeup(60 * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}