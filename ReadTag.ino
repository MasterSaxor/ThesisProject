#include <Arduino.h>
#include <SoftwareSerial.h>
SoftwareSerial RFID(2, 3);
byte b;
byte buffer[30];

uint8_t idx;
boolean started = false;

byte XOR;
byte inverted;

uint64_t value;


void print_uint64_t(uint64_t num) {

  char rev[128]; 
  char *p = rev+1;

  while (num > 0) {
    *p++ = '0' + ( num % 10);
    num/= 10;
  }
  p--;
  /*Print the number which is now in reverse*/
  while (p > rev) {
    Serial.print(*p--);
  }
}


void setup()
{
  Serial.begin (9600);
  RFID.begin (9600);
//  pinMode(resetPin, OUTPUT);
//  digitalWrite(resetPin, LOW);
  //digitalWrite(resetPin, LOW);
  //pinMode(12, OUTPUT);
//  digitalWrite(12, HIGH);
//  delay(3000);
//  digitalWrite(12, LOW);
}

void loop()
{
  
  
  while (RFID.available())
  {
    b = RFID.read();

    if (b == 0x02) // Start byte
    {
      idx = 0;
      started = true;
    }


    if (started) // Ignore anything received until we get a start byte.
    {
      buffer[idx++] = b;

      if (b == 0x03) // End byte
      {
        started = false;
        
        
        // Display the received data.
        Serial.print("Received data: ");
        for (int x = 0; x < idx; x++)
        {
          if (buffer[x] < 0x10)
          {
            Serial.print("0"); // Pad with leading 0
          }
          Serial.print (buffer[x], HEX);
          Serial.print (" ");
        }
        Serial.println("");


        // Check we received the message ok.  XOR checksum on bytes 01-26 should match byte 27.
        XOR = buffer[1];
        for (int x = 2; x <= 26; x++)
        {
          XOR ^= buffer[x];  
        }
        Serial.print("Calculated checksum: ");
        Serial.print(XOR, HEX);

        if (XOR == buffer[27])
          Serial.println(" (Correct)");
        else
          Serial.println(" (Error)");


        // Check the inverted XOR checksum
        inverted = ~XOR;
        Serial.print("Inverted checksum: ");
        Serial.print(inverted, HEX);
        if (inverted == buffer[28])
          Serial.println(" (Correct)");
        else
          Serial.println(" (Error)");

        
        // Extract the card number from bytes 01 (LSB) - 10 (MSB).
        value = 0;
        Serial.print("This is my buffer[x] from 1 to 10: ");
        for (int x = 10; x >= 1; x--)
        {
          Serial.print(buffer[x], HEX);
          if(buffer[x] <= '9')
            value = (value<<4) + buffer[x] - '0';  
          else
            value = (value<<4) + buffer[x] - '7';  
        }
        Serial.println("");

        Serial.print("Card number: ");
        print_uint64_t(value);


        // Extract the country number from bytes 11 (LSB) - 14 (MSB).
        value = 0;
        for (int x = 14; x >= 11; x--)
        {
          if(buffer[x] <= '9')
            value = (value<<4) + buffer[x] - '0';
          else
            value = (value<<4) + buffer[x] - '7';  
        }
        //philippines' country code = 608
        Serial.print("Country number: ");
        print_uint64_t(value);


        // Extract the Data Block from byte 15.
        Serial.print("Data block: ");
        Serial.println(buffer[15] - '0');

        
        // Extract the Animal Flag from byte 16.
        Serial.print("Animal flag: ");
        Serial.println(buffer[16] - '0');

        Serial.println("\r");

        delay(5000);
        digitalWrite(12, LOW);
      }
    }
  }
}
