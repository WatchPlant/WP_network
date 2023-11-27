#include <Arduino.h>
#include <HardwareSerial.h>
#include <loopTimer.h>

#include "zigbee.h"
#include "utils.h"
#include "lorawan.h"

#define MAXIMUM_BUFFER_SIZE 300
#define START_DELIMITER     0x7E
#define RST_PIN             PA0


HardwareSerial xbee(1);


// Define RX and TX global buffer
char tx_buf[MAXIMUM_BUFFER_SIZE] = {0};
char rx_buf[MAXIMUM_BUFFER_SIZE] = {0};
int tx_length = 0;

// Initial address of the sink
uint64_t sink_addr = 0x0013a20041f223b8;
int zigbeeFailed = 0;

// Delays for software-"multithreading"/scheduling
millisDelay sendDelay;

int measurement_interval = 5000;


/* Send the number of failed messages to LoRaWAN. */ 
static void prepareTxFrame(int value, uint8_t port )
{
  /*appData size is LORAWAN_APP_DATA_MAX_SIZE which is defined in "commissioning.h".
  *appDataSize max value is LORAWAN_APP_DATA_MAX_SIZE.
  *if enabled AT, don't modify LORAWAN_APP_DATA_MAX_SIZE, it may cause system hanging or failure.
  *if disabled AT, LORAWAN_APP_DATA_MAX_SIZE can be modified, the max value is reference to lorawan region and SF.
  *for example, if use REGION_CN470, 
  *the max value for different DR can be found in MaxPayloadOfDatarateCN470 refer to DataratesCN470 and BandwidthsCN470 in "RegionCN470.h".
  */ 
    appDataSize=1;
    appData[0] = value;
}

/* Receive LoRaWAN messages. */
void downLinkDataHandle(McpsIndication_t *mcpsIndication)
{
  Serial.printf("+REV DATA:%s,RXSIZE %d,PORT %d\r\n",mcpsIndication->RxSlot?"RXWIN2":"RXWIN1",mcpsIndication->BufferSize,mcpsIndication->Port);
  Serial.print("+REV DATA:");
  uint8_t identifier = mcpsIndication->Buffer[0];
  Serial.printf("identifier: %02x\n", identifier);
  sendDelay.stop();

  // User-defined identifiers: 0x00: new sink address, 0x01: new measurement interval.
  // This type of node broadcasts the newly received data to other nodes over Zigbee.
  if (identifier == 0x00)
  {
    sink_addr = 0;
    for(int i=1;i<mcpsIndication->BufferSize;i++)
    {
      sink_addr |= (uint64_t)mcpsIndication->Buffer[i] << ((8-i)*8);
    }
    tx_length = writeFrame(tx_buf, 0x01, 0xFFFF, 0xFFFFFFFFFFFFFFFF, (char*)mcpsIndication->Buffer, mcpsIndication->BufferSize);
    xbee.write(tx_buf, tx_length);
    zigbeeFailed = 0;
  }
  else if(identifier == 0x01)
  {
    measurement_interval = 0;
    for(int i=1;i<mcpsIndication->BufferSize;i++)
    {
      measurement_interval |= (int)mcpsIndication->Buffer[i] << ((4-i)*8);
    }
    tx_length = writeFrame(tx_buf, 0x01, 0xFFFF, 0xFFFFFFFFFFFFFFFF, (char*)mcpsIndication->Buffer, mcpsIndication->BufferSize);
    xbee.write(tx_buf, tx_length);
  }
  else
  {
    for(uint8_t i=0;i<mcpsIndication->BufferSize;i++)
    {
      Serial.printf("%02X",mcpsIndication->Buffer[i]);
    }
    Serial.println();
  }
  sendDelay.start(measurement_interval);
}


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
    //Serial.printf("Payload Size: %i\n", length);
    if(length <= 0)
        return;

    /*
    for (int i = 0 ; i<length; i++)
    {
        Serial.printf("%02x", buffer[i]);
    }
    Serial.printf("\n");
    */

    if(result.frameID == 0x90)  // Frame type: RX packet
    {
      Serial.printf("%.*s\n", length - 12, buffer + 12);
    }
    else if(result.frameID == 0x8b)  // Frame type: Zigbee transmit status
    {
      if(buffer[5] != 0x00)
      {
        zigbeeFailed++;
      }
    }
  }
  return;
}

void setup() {
    Serial.begin(115200);
    xbee.begin(115200, SERIAL_8N1, 12, 13);
    digitalWrite(15, LOW); 
    delay(100);
    digitalWrite(15, HIGH); 
    delay(100);

    convert_keystring();
    Mcu.begin();
    deviceState = DEVICE_STATE_INIT;
    
    sendDelay.start(measurement_interval);
    Serial.printf("Setup finished\n");
}

/* Send Zigbee message. */
void sendMessage()
{
  if (sendDelay.justFinished())
  {
    sendDelay.repeat();
    Serial.printf("Send message\n");
    char payload[] = "L";
    tx_length = writeFrame(tx_buf, 0x01, 0xFFFE, sink_addr, payload, sizeof(payload)-1);
    xbee.write(tx_buf, tx_length);
  }
}


void loop() {
  sendMessage();
  /*
  Serial.printf("Payload length: %i\n", tx_length);
  for (int i = 0 ; i<tx_length; i++)
  {
    Serial.printf("%02x", tx_buf[i]);
  }
  Serial.printf("\n");
  */
  rx_callback(rx_buf);

  switch( deviceState )
  {
    case DEVICE_STATE_INIT:
    {
#if(LORAWAN_DEVEUI_AUTO)
      LoRaWAN.generateDeveuiByChipID();
#endif
      LoRaWAN.init(loraWanClass,loraWanRegion);
      break;
    }
    case DEVICE_STATE_JOIN:
    {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND:
    {
      prepareTxFrame(zigbeeFailed, appPort );
      zigbeeFailed = 0;
      LoRaWAN.send();
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE:
    {
      // Schedule next packet transmission
      txDutyCycleTime = appTxDutyCycle + randr( -APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND );
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP:
    {
      LoRaWAN.sleep(loraWanClass);
      break;
    }
    default:
    {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}