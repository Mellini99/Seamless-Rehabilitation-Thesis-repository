#include <EEPROM.h>
#include "uRTCLib.h"

#define DS3231_I2C_ADDRESS 0x68
uRTCLib rtc(DS3231_I2C_ADDRESS);

struct datestamp_t {
    uint16_t magic;         // magic number (see MAGIC)
    uint8_t day;            // day of the month [1, 31]
};

struct header_t {
  uint16_t magic;           // magic number (see MAGIC)
  uint8_t day;              // last recorded day of the month [1, 31]
  uint8_t month;            // last recorded month [1, 12]
  uint16_t HEAD;            // pointer to the head of EEPROM memory
};

struct session_record_t {
  uint8_t timeslot;         // 24-hour day is divided into 10-minute intervals for memory efficiency.
  uint8_t repetition_count; // number of repetitions measured in this session.
};

void print_rtc() {
  rtc.refresh();

  Serial.print(F("RTC: "));
  Serial.print(rtc.hour());
  Serial.print(':');
  Serial.print(rtc.minute());
  Serial.print(' ');
  Serial.print(rtc.day());
  Serial.print('-');
  Serial.print(rtc.month());
  Serial.print('-');
  Serial.println(rtc.year());
}

void print_header(header_t* header) {
  Serial.println(F("-- HEADER --"));
  Serial.print(F("Magic:\t"));
  Serial.println(header->magic, HEX);
  Serial.print(F("Day:\t"));
  Serial.println(header->day);
  Serial.print(F("Month:\t"));
  Serial.println(header->month);
  Serial.print(F("HEAD:\t"));
  Serial.println(header->HEAD);
  Serial.println(F("-- END HEADER --"));
}

void init_rtc() {

  URTCLIB_WIRE.begin();

  rtc.refresh();

  if (rtc.year() != 0) {

    Serial.println(F("RTC time found!"));

    print_rtc();

  } else {
  
    Serial.println(F("No RTC time found."));
    print_rtc();
  }
}

void setup() {

  Serial.begin(9600);

  delay(1000);

  init_rtc();

  Serial.println("Done with RTC for now.");
  

  // expect a header at 0x00
  header_t header;
  EEPROM.get(0, header);

  print_header(&header);

  datestamp_t datestamp;
  datestamp.day = header.day;

  int session_count = 0;

  for (int i = sizeof(header_t); i < header.HEAD;) {

    Serial.println(i);

    // peek at the bytes
    uint16_t magic;
    EEPROM.get(i, magic);

    if (magic == 0xABBAu) {
    
      Serial.print("Datestamp found at ");
      Serial.print(i);
    
      EEPROM.get(i, datestamp);
      i += sizeof(datestamp_t);
    
      Serial.print('[');
      Serial.print(datestamp.day);
      Serial.println(']');
    
    } else {

      // must be a session record
      session_record_t session_record;
      EEPROM.get(i, session_record);
      i += sizeof(session_record_t);

      Serial.print("Session record: ");

      Serial.print(session_record.repetition_count);
      Serial.print(" reps at [");
      Serial.print(session_record.timeslot);
      Serial.print("] on date ");
      Serial.println(datestamp.day);

      session_count++;
    }
  }

  Serial.print("Done. Found ");
  Serial.print(session_count);
  Serial.println(" sessions.");
}

void loop() {
}
