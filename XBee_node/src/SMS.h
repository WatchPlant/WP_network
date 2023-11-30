#include "Adafruit_FONA.h"
#include <HardwareSerial.h>


HardwareSerial *fonaSerial = &Serial2;


// this is a large buffer for replies
char replybuffer[255];

char fonaNotificationBuffer[64];          //for notifications from the FONA
char smsBuffer[250];


// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

void setup_SMS() {

  // make it slow so its easy to read!
  fonaSerial->begin(115200, SERIAL_8N1, FONA_TX, FONA_RX);
  if (! fona.begin(*fonaSerial)) {
    Serial.printf("Couldn't find FONA\n");
    while(1);
  }

  char PIN[5] = "6031";
  if (! fona.unlockSIM(PIN)) {
    Serial.println(F("Failed to unlock SIM card!"));
  } else {
    Serial.println(F("SIM card unlocked!"));
  }


  fonaSerial->print("AT+CNMI=2,1\r\n");  //set up the FONA to send a +CMTI notification when an SMS is received

  Serial.printf("FONA Ready\n");
}

void send_SMS(char* recepient, char* message)
{
  Serial.printf("Sending SMS to %s\n", recepient);
  if (!fona.sendSMS(recepient, message)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("Sent!"));
  }
}


void loop_SMS() 
{

  int8_t smsnum = fona.getNumSMS();
  if (smsnum < 0) {
    Serial.println(F("Could not read # SMS"));
  } else {
    Serial.print(smsnum);
    Serial.println(F(" SMS's on SIM card!"));
  }
  
  char* bufPtr = fonaNotificationBuffer;    //handy buffer pointer
  
  if (fona.available())      //any data available from the FONA?
  {
    int slot = 0;            //this will be the slot number of the SMS
    int charCount = 0;
    //Read the notification into fonaInBuffer
    do  {
      *bufPtr = fona.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (fona.available()) && (++charCount < (sizeof(fonaNotificationBuffer)-1)));
    
    //Add a terminal NULL to the notification string
    *bufPtr = 0;

    //Scan the notification string for an SMS received notification.
    //  If it's an SMS message, we'll get the slot number in 'slot'
    if (1 == sscanf(fonaNotificationBuffer, "+CMTI: " FONA_PREF_SMS_STORAGE ",%d", &slot)) {
      Serial.print("slot: "); Serial.println(slot);
      
      char callerIDbuffer[32];  //we'll store the SMS sender number in here
      
      // Retrieve SMS sender address/phone number.
      if (! fona.getSMSSender(slot, callerIDbuffer, 31)) {
        Serial.println("Didn't find SMS message in slot!");
      }
      Serial.print(F("FROM: ")); Serial.println(callerIDbuffer);

        // Retrieve SMS value.
        uint16_t smslen;
        if (fona.readSMS(slot, smsBuffer, 250, &smslen)) { // pass in buffer and max len!
          Serial.println(smsBuffer);
        }
      
      // delete the original msg after it is processed
      //   otherwise, we will fill up all the slots
      //   and then we won't be able to receive SMS anymore
      if (fona.deleteSMS(slot)) {
        Serial.println(F("OK!"));
      } else {
        Serial.print(F("Couldn't delete SMS in slot ")); Serial.println(slot);
        fona.print(F("AT+CMGD=?\r\n"));
      }
    }
  }
}