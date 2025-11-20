#include <EEPROM.h>
#include "uRTCLib.h"

#define PROD                                // set to DEBUG to print debug messages

#define DS3231_I2C_ADDRESS 0x68
#define PIN_MOTOR 6
#define PIN_SENSOR_TOP    A1                // top sensor
#define PIN_SENSOR_BOTTOM A0                // bottom sensor
#define EEPROM_MIN_LENGTH 512//b            // minimum number of EEPROM bytes we expect
#define SESSION_WINDOW_DURATION 300000//ms  // a session window is 5 minutes
#define CALIBRATION_SAMPLE_COUNT 10         // number of samples to 
#define CALIBRATION_DURATION 1000//ms       // duration of sensor calibration
#define SENSOR_THRESHOLD_OFFSET 30
#define MOTOR_INIT_DURATION 500//ms        // the motor active duration of the first repetition in a session
#define MOTOR_DURATION 200//ms              // motor active duration
#define RECALIBRATION_INTERVAL 1800000//ms  // recalibration interval of proximity sensors (30 minutes)

#define MAGIC 0xABBAu

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

session_record_t* session_record = NULL;
header_t* header = NULL;
unsigned long session_rolling_window = 0;

int sensor1_threshold = 0;
int sensor2_threshold = 0;
bool sensor1_detected = false;

unsigned long motor_end_millis = 0;
unsigned long last_calibration_millis = 0;

void print_header() {
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

void print_session() {

  if (session_record == NULL) {
    Serial.println(F(">> SESSION <<"));
    Serial.println(F("NULL"));
    Serial.println(F("<> END SESSION <>"));
    return;
  }

  Serial.println(F(">> SESSION <<"));
  Serial.print(F("Slot:\t"));
  Serial.println(session_record->timeslot);
  Serial.print(F("Reps.:\t"));
  Serial.println(session_record->repetition_count);
  Serial.println(F("<> END SESSION <>"));
}

void init_rtc() {

  URTCLIB_WIRE.begin();

  rtc.refresh();

  if (rtc.year() == 0) {
    #ifdef DEBUG
    Serial.println(F("Error: RTC not set! Setting RTC to 00:00:00, 1/1/2001..."));
    #endif
    rtc.set(0, 0, 0, 2, 1, 1, 1);
    rtc.refresh();
  }

  #ifdef DEBUG
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
  #endif
}

void setup_pins() {

  pinMode(PIN_MOTOR, OUTPUT);
  digitalWrite(PIN_MOTOR, LOW);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);

  pinMode(LED_BUILTIN, OUTPUT);
}

void datestamp() {

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

bool session_valid() {
  
  if (session_record == NULL) {
    return false;
  }

  return session_record->repetition_count > 1;
}

void store_session_record() {

  #ifdef DEBUG
  print_session();
  #endif

  #ifdef DEBUG
  Serial.print(F("Storing session at "));
  print_header();
  #endif

  EEPROM.put(header->HEAD, *session_record);
  header->HEAD += sizeof(session_record_t);

  #ifdef DEBUG
  Serial.print(F("New header: "));
  print_header();
  #endif

  EEPROM.put(0x00, *header);
}

void init_store() {

  #ifdef DEBUG
  int eeprom_size = EEPROM.length();

  if (eeprom_size < EEPROM_MIN_LENGTH) {
    Serial.print(F("Warning: expected at least 512 bytes of memory, got "));
    Serial.println(eeprom_size);
  }
  #endif

  header = (header_t*)malloc(sizeof(header_t));

  EEPROM.get(0, *header);

  if (header->magic != MAGIC) {

    Serial.println(F("No magic found. Adding magic..."));
    
    rtc.refresh();

    header->magic = MAGIC;
    header->day = (uint8_t)rtc.day();
    header->month = (uint8_t)rtc.month();
    header->HEAD = sizeof(header_t);

    // write header EEPROM
    EEPROM.put(0x00, *header);
  }

  #ifdef DEBUG
  Serial.print(F("EEPROM initialized. Available: "));
  Serial.print(eeprom_size);
  Serial.print("b, Used: ");
  Serial.print(header->HEAD);
  Serial.println("b.");

  print_header();
  #endif
}

// take 10 measurements over a 2 seconds period to determine average exposure
void calibrate_sensors() {
  
  if (session_record != NULL) {
    return; // do not calibrate during a session
  }

  digitalWrite(LED_BUILTIN, HIGH);

  int s1_total = 0;
  int s2_total = 0;
  const int calibration_interval = CALIBRATION_DURATION / CALIBRATION_SAMPLE_COUNT;

  for (int i = 0; i < CALIBRATION_SAMPLE_COUNT; ++i) {
    
    // read sensors
    const int s1 = analogRead(PIN_SENSOR_TOP);  // Read the analog value from the phototransistor
    const int s2 = analogRead(PIN_SENSOR_BOTTOM);  // Read the analog value from the phototransistor

    // total average
    s1_total += s1;
    s2_total += s2;

    // delay by interval
    delay(calibration_interval);
  }

  sensor1_threshold = (s1_total / CALIBRATION_SAMPLE_COUNT) + SENSOR_THRESHOLD_OFFSET;
  sensor2_threshold = (s2_total / CALIBRATION_SAMPLE_COUNT) + SENSOR_THRESHOLD_OFFSET;

  last_calibration_millis = millis();

  #ifdef DEBUG
  Serial.print(F("Calibration done. T_s1: "));
  Serial.print(sensor1_threshold);
  Serial.print(F(", T_s2: "));
  Serial.println(sensor2_threshold);
  #endif

  digitalWrite(LED_BUILTIN, LOW);
}

void setup() {
  
  #ifdef DEBUG
  Serial.begin(9600);
  #endif

  setup_pins();

  init_rtc();
  init_store();

  calibrate_sensors();
}

// returns RTC time over the interval [00:00, 23:50] that is divided into 10 minute slots. Slot 0 is 00:00.
uint8_t get_current_timeslot() {
  
  rtc.refresh();
  
  uint16_t a = rtc.hour() * (uint16_t)60;
  uint16_t b = a + rtc.minute();
  uint8_t slot = b / 10;

  return slot;
}

void add_repetition() {

  uint8_t current_timeslot = get_current_timeslot();

  if (session_record == NULL) {

    #ifdef DEBUG
    Serial.println(F("Allocating new session!"));
    #endif
    
    session_record = (session_record_t*)malloc(sizeof(session_record_t));
    session_record->timeslot = current_timeslot;
    session_record->repetition_count = 0;

    #ifdef DEBUG
    print_session();
    #endif

    session_rolling_window = millis();
  }

  session_record->repetition_count += 1;
}

void loop() {

  if (motor_end_millis > 0 && millis() > motor_end_millis) {
    digitalWrite(PIN_MOTOR, LOW); // stop motor
    motor_end_millis = 0;
  }

  // check session window
  if (session_record != NULL && millis() - session_rolling_window > SESSION_WINDOW_DURATION) {
    // session window has closed -- store the session and invalidate

     rtc.refresh();

    if (header->day < rtc.day()) {
      // a new day has started! Let's leave a date stamp before we continue
      datestamp();
    }
    
    if (session_valid()) {
      
      store_session_record();

    } else {

      calibrate_sensors();
    }
    
    // cleanup
    free(session_record);
    session_record = NULL;
  }

  if (millis() - last_calibration_millis > RECALIBRATION_INTERVAL) {

    calibrate_sensors();
  }

  const int s1 = analogRead(PIN_SENSOR_TOP);
  const int s2 = analogRead(PIN_SENSOR_BOTTOM);
  
  if (s1 > sensor1_threshold) {

    if (s2 > sensor2_threshold) {
      // both HIGH -> recalibrate!
      calibrate_sensors();
      return;
    }

    sensor1_detected = true;
    #ifdef DEBUG
    Serial.print("s1: ");
    Serial.println(s1);
    #endif
  }

  if (sensor1_detected && s2 > sensor2_threshold) {
    
    #ifdef DEBUG
    Serial.println("Repetition detected!");
    #endif

    add_repetition();

    #ifdef DEBUG
    print_session();
    #endif

    digitalWrite(PIN_MOTOR, HIGH); // activate motor
    motor_end_millis = millis() + (session_valid() ? MOTOR_DURATION : MOTOR_INIT_DURATION); // first motor activiation in a session should be longer
  
    // Reset for next repetition
    sensor1_detected = false;
  } 
    
  delay(50);
}