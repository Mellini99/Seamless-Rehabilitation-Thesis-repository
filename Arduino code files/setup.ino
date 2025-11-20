#include <EEPROM.h>
#include "uRTCLib.h"

#define DS3231_I2C_ADDRESS 0x68
#define MAGIC 0xABBAu
#define EEPROM_MIN_LENGTH 512//b            // minimum number of EEPROM bytes we expect

#define HOUR    19  // hour of day
#define MINUTE  27  // minute of hour
#define DAY     20  // day of month
#define MONTH   9  // month of year
#define YEAR    25  // year (last two digits!) I.e. 2025 should be defined as 25.

#define FORCE_OVERWRITE false     // CAREFUL: set to true ONLY if you want to overwrite an existing header. This prevents accidentally overwriting an arduino with existing data.

uRTCLib rtc(DS3231_I2C_ADDRESS);

struct header_t {
  uint16_t magic;           // magic number (see MAGIC)
  uint8_t day;              // last recorded day of the month [1, 31]
  uint8_t month;            // last recorded month [1, 12]
  uint16_t HEAD;            // pointer to the head of EEPROM memory
};

struct datestamp_t {
    uint16_t magic;         // magic number (see MAGIC)
    uint8_t day;            // day of the month [1, 31]
};

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

void init_rtc() {

  URTCLIB_WIRE.begin();

  rtc.refresh();

  if (rtc.year() != 0) {

    Serial.println(F("RTC time found!"));

    print_rtc();

    if (FORCE_OVERWRITE) {

      Serial.println(F("FORCEFULLY OVERWRITING EXISTING RTC in 10 seconds... (I hope you know what you're doing...)"));
      delay(10000);

    } else {

      Serial.println(F("Overwrite protection enabled. No RTC is set. If you wish, you can FORCEFULLY overwrite the RTC by setting FORCE_OVERWRITE."));
      return;
    }
  
  } else {

    Serial.println(F("No RTC time found!"));
  }

  int year = YEAR;
  if (year > 2000) {
      Serial.println(F("Error: YEAR definition should be 2 digits! I'm gonna subtract 2000."));
      year -= 2000;
      Serial.print(F("Setting YEAR to "));
      Serial.println(year);
  }

  Serial.print(F("Setting RTC to "));
  Serial.print(HOUR);
  Serial.print(':');
  Serial.print(MINUTE);
  Serial.print(' ');
  Serial.print(DAY);
  Serial.print('-');
  Serial.print(MONTH);
  Serial.print('-');
  Serial.print(YEAR);
  Serial.println(F("..."));

  rtc.set(0, MINUTE, HOUR, 1, DAY, MONTH, year); // day of week is always monday...
  
  print_rtc();
}

void datestamp(header_t* header) {

  rtc.refresh();

  datestamp_t datestamp = {
      MAGIC,
      (uint8_t)rtc.day()
  };
  
  // write datestamp to EEPROM
  EEPROM.put(header->HEAD, datestamp);
  header->HEAD += sizeof(datestamp_t);

  // update header
  header->day = (uint8_t)rtc.day();
  header->month = (uint8_t)rtc.month();

  EEPROM.put(0x00, *header);
}

void init_header() {

  int eeprom_size = EEPROM.length();

  if (eeprom_size < EEPROM_MIN_LENGTH) {
    Serial.print(F("Warning: expected at least 512 bytes of memory, got "));
    Serial.println(eeprom_size);
  }

  header_t* header = (header_t*)malloc(sizeof(header_t));

  EEPROM.get(0, *header);

  if (header->magic == MAGIC) {

    Serial.println(F("Header found!"));

    print_header(header);

    if (FORCE_OVERWRITE) {
      Serial.println(F("FORCEFULLY OVERWRITING EXISTING HEADER in 10 seconds... (I hope you know what you're doing...)"));
      delay(10000);
    } else {
      Serial.println(F("Overwrite protection enabled. No data is lost. If you wish, you can FORCEFULLY overwrite the header by setting FORCE_OVERWRITE."));
      return;
    }
  }

  Serial.println(F("Writing initial header..."));

  rtc.refresh();
  
  header->magic = MAGIC;
  header->day = (uint8_t)rtc.day();
  header->month = (uint8_t)rtc.month();
  header->HEAD = sizeof(header_t);

  // write header EEPROM
  // EEPROM.put(0x00, *header); // already written by datestamp()

  datestamp(header);

  print_header(header);

  Serial.print(F("EEPROM initialized. Available: "));
  Serial.print(eeprom_size);
  Serial.print("b, Used: ");
  Serial.print(header->HEAD);
  Serial.println("b.");

  free(header);
  header = NULL;
}

void setup() {
  
  Serial.begin(9600);

  init_rtc();
  init_header();

}

void loop() {
}
