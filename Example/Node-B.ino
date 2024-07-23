/* Heltec Automation Receive communication test example
 *
 * Function:
 * 1. Receive the same frequency band lora signal program
 *
 * Description:
 *
 * HelTec AutoMation, Chengdu, China
 * 成都惠利特自动化科技有限公司
 * www.heltec.org
 *
 * this project also realess in GitHub:
 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
 * */

#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "HT_SSD1306Wire.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Wire.h>

#include "config.h"

WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wm;
bool res;

#define RF_FREQUENCY 433000000 // Hz   433 for Asia

#define TX_OUTPUT_POWER 20 // dBm

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

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];
char data[30];

static RadioEvents_t RadioEvents;

int16_t txNumber;

int16_t rssi, rxSize;

bool lora_idle = true;

extern SSD1306Wire display;

// Connect wifi
void connectwifi()
{
  // ESP32 start as AP mode
  res = wm.autoConnect(ssid, password);

  // If failed to connect wifi. press reset wifi button for start AP mode again
  if (!res)
  {
    wm.resetSettings();
    Serial.println("Failed to connect");
    // resetWifi();
  }
  else
  {
    Serial.println("connected");
    Serial.println(WiFi.SSID());
  }
}

// Convert mac to String
String macToStr(const uint8_t *mac)
{
  String result;
  for (int i = 0; i < 6; ++i)
  {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

// Connect Mqtt
void connectmqtt()
{
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientName;
    clientName += "Heltec Wifi LoRa V2-";
    uint8_t mac[6];
    WiFi.macAddress(mac);
    clientName += macToStr(mac);
    clientName += "-";
    clientName += String(micros() & 0xff, 16);
    Serial.print("Connecting to ");
    Serial.print(mqtt_server);
    Serial.print(" as ");
    Serial.println(clientName);

    // Attempt to connect
    // If you want to use a username and password, change next line to
    if (client.connect((char *)clientName.c_str()))
    // if (client.connect("esp32-s", mqttUser, mqttPass))
    {
      Serial.println("connected");
      digitalWrite(LED_BUILTIN, LOW);
      display.clear();
      display.drawString(0, 10, "Wifi: Connected        ");
      display.drawString(0, 20, "Mqtt: Connected        ");
      display.display();
      delay(3000);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      digitalWrite(LED_BUILTIN, HIGH);
      delay(5000);
    }
  }
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    data[i] += (char)message[i];
  }
  Serial.println();
}

void setup()
{
  Serial.begin(115200);
  Mcu.begin();

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  display.clear();

  display.drawString(0, 0, "Heltec.LoRa Initial success!");
  display.drawString(0, 10, "Please Setup Wifi");
  display.drawString(0, 20, "SSID: LoRa_P2P");
  display.display();
  delay(1000);


  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  RadioEvents.RxDone = OnRxDone;
  Radio.Init(&RadioEvents);
  Radio.SetChannel(RF_FREQUENCY);
  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  connectwifi();
  connectmqtt();
  client.subscribe(SUB_Topic);
}

void loop()
{

  if (!client.connected())
  {
    connectmqtt();
  }

  client.loop();

  display.clear();

  // Wait for receive message from node A
  if (lora_idle)
  {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }

  Radio.IrqProcess();
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
  rssi = rssi;
  rxSize = size;
  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  Radio.Sleep();

  // Print data from node A
  Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n", rxpacket, rssi, rxSize);

  // Calculate the length of the byte array
  String receivedData = String(rxpacket).c_str();
  size_t byteArrayLength = receivedData.length() / 2;
  byte byteArray[byteArrayLength];

  // Convert the hex string to a byte array
  for (size_t i = 0; i < byteArrayLength; ++i)
  {
    // Extract two characters from the string
    String byteString = receivedData.substring(i * 2, i * 2 + 2);
    // Convert the extracted characters to a byte
    byteArray[i] = (byte)strtol(byteString.c_str(), NULL, 16);
  }

  // Calculate the checksum
  byte c = calculateChecksum8(byteArray, byteArrayLength);
  byteArray[byteArrayLength] = c;

  Serial.print("Byte Array: ");
  for (size_t i = 0; i < byteArrayLength; ++i)
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

  Serial.println();
  if (receivedData.length() == 4)
  {

    int id = 1;
    int water = ((byteArray[0] * 256) + byteArray[1]);
    float waterLevel = ((float)water / 1000);
    String checksum = String(c, HEX).c_str();

    String ID = String(id).c_str();
    String WaterLevel = String(waterLevel, 2).c_str();
    String Checksum = String(c).c_str();
    String RSSI = String(rssi).c_str();
    String RxSize = String(rxSize).c_str();

    // Manage Payload for node-red
    String payload = "{\"ID\": " + ID + ", \"Water Level\": " + WaterLevel + ", \"checksum\": " + Checksum + ", \"Rssi\": " + RSSI + ", \"Rxsize\": " + RxSize + "}";
    char msg_data[90];

    payload.toCharArray(msg_data, (payload.length() + 1));
    client.publish(PUB_Topic, msg_data);

    Serial.println(payload);

    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);

    String w_OLED = WaterLevel + " m";
    String result = "01" + String(rxpacket) + checksum;

    display.drawString(0, 10, "Data: ");
    display.drawString(35, 10, result);
    display.drawString(0, 20, "Water level: ");
    display.drawString(60, 20, w_OLED);
    display.drawString(0, 30, "Rssi: ");
    display.drawString(35, 30, RSSI);
    display.display();
  }

  // Send handcheck back to Node A 
  int a = -1;
  sprintf(txpacket, "%d", a);
  Radio.Send((uint8_t *)txpacket, strlen(txpacket)); 

  lora_idle = true;
}

void OnTxDone(void)
{
  Serial.println("TX done......");
  lora_idle = true;
}

void OnTxTimeout(void)
{
  Radio.Sleep();
  Serial.println("TX Timeout......");
  lora_idle = true;
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