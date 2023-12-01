#include <Arduino.h>
#include <HardwareSerial.h>
#include <loopTimer.h>

#ifdef TARGET_STM_32
#include "stm32wbxx_hal.h"
#endif

#ifdef IKS01A3
#include "IKS01A3.h"
#endif

#ifdef SMS
#include "SMS.h"
#endif

#include "zigbee.h"
#include "utils.h"

#define MAXIMUM_BUFFER_SIZE 300
#define START_DELIMITER     0x7E
#define RST_PIN             PA0


#ifdef TARGET_STM_32
HardwareSerial xbee(TX_PIN, RX_PIN);
#elif TARGET_ESP_32
HardwareSerial xbee(1);
#endif


// Define RX and TX global buffer
char tx_buf[MAXIMUM_BUFFER_SIZE] = {0};
char rx_buf[MAXIMUM_BUFFER_SIZE] = {0};
int tx_length = 0;

// Initial address of the sink
uint64_t sink_addr = 0x0013a20041f223b2;

// Delays for software-"multithreading"/scheduling
millisDelay sendDelay;
int measurement_interval = 5000;


/*
TODO
- Change void to int, return error codes
- Convert to a proper callback
*/

/* Receive Zigbee messages. */
static void rx_callback(char *buffer)
{
  int length = 0;
  int timeoutCnt = 0;
  char c;
  parsedFrame result;

  // Wait until a frame delimiter arrives.
  if(xbee.available()>0)
  {
    c = 0;
    while(c != START_DELIMITER)
    {
      if(waitForByte(xbee)==0)
          return;
      c = xbee.read();
    }
    // Parse the frame. In case of an error, a negative length is returned.
    result = readFrame(buffer, xbee);
    length = result.length;
    
    // NOTE: Received messages include transmit status packets and similar.
    Serial.printf("Received message:\n");
    Serial.printf("\tSize: %i\n", length);
    Serial.printf("\tFrame ID: %02x\n", result.frameID);
    Serial.printf("\tPayload: ");
    print_hex(buffer, length);
    Serial.printf("\n");
    
    if(length <= 0)
      return;

    if(result.frameID == 0x90)  // Frame type: RX packet
    {
      sendDelay.stop();
      Serial.printf("%.*s\n", length - 12, buffer + 12);
      char identifier = buffer[12];
      Serial.printf("identifier: %02x\n", identifier);
      
      // User-defined identifiers: 0x00: new sink address, 0x01: new measurement interval.
      // This type of node stores the newly received data and ajdusts its behaviour accordingly.
      if (identifier == 0x00)
      {
        sink_addr = 0;
        for(int i=0;i<8;i++)
        {
          sink_addr |= (uint64_t)buffer[i+13] << ((7-i)*8);
        }
      }
      else if (identifier == 0x01)
      {
        measurement_interval = 0;
        for(int i=0;i<4; i++)
        {
          measurement_interval |= (int)buffer[i+13] << ((3-i)*8);
        }
      }
#ifdef SMS
      else if (identifier == 0x04)
      {
        char phone_number[30];
        char message[150];
        int phone_length = buffer[13];

        sprintf(phone_number, "%.*s", phone_length, buffer + 14);
        sprintf(message, "%s", buffer + 14 + phone_length);

        send_SMS(phone_number, message);
      }
#endif
      sendDelay.start(measurement_interval);
    }
  }
}


void setup() {
  Serial.begin(115200);
#ifdef TARGET_STM_32
  xbee.begin(115200);
#elif TARGET_ESP_32
  xbee.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
#endif
  delay(100);

#ifdef IKS01A3
  setup_sensors();
#endif

#ifdef SMS
  setup_SMS();
#endif

  sendDelay.start(measurement_interval);
  Serial.printf("Setup finished\n");
}

/* Send Zigbee message. */
void sendMessage()
{
  int length;
  if (sendDelay.justFinished())
  {
    sendDelay.repeat();
    char payload[99];
    payload[0] = 0x03;
#ifdef IKS01A3
    length = read_sensors(payload);
#else
    length = sprintf(&payload[1], "%.2f,%.2f,%.1f,%.1f,%.1f,%.1f", 21.32, 30.56, 999.1, 20.1, 30.2, 40.3);
#endif
    Serial.printf("Send message:\n");
    Serial.printf("\tSize: %d\n", length);
    Serial.printf("\tPayload: %s\n", payload);
    Serial1.printf("\n");
    tx_length = writeFrame(tx_buf, 0x01, 0xFFFE, sink_addr, payload, length + 1);
    xbee.write(tx_buf, tx_length);
  }
}


void loop() {
  sendMessage();
  rx_callback(rx_buf);
#ifdef SMS
  // loop_SMS();
#endif
}
