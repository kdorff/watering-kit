/**************************************************
 * This code has been tested with the Elecrow
 * watering kit that has an integrated 
 * Arduino Leonardo.
 * 
 * Make sure to set your Board and Port 
 * appropriatly. See the README.md for programming
 * notes.
 **************************************************/

#include <Wire.h>
#include <U8g2lib.h>
#include <U8x8lib.h>
#include <RTClib.h>
#include <VL53L0X.h>

// Review all of the values found here.
// (See watering-kit-config.h-sample)
#include "watering-kit-config.h"

U8G2_SH1106_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0, /* clock=*/SCL, /* data=*/SDA, /* reset=*/U8X8_PIN_NONE);
RTC_DS1307 RTC;

// Time (millis()) we last sent stats
unsigned long send_stats_last = 0;

// If we want to force sending stats on this iteration
bool send_stats_force = false;

// The number of sensors. If you want more, you will need
// to many of the below arrays, too.
int num_sensors = 4;

// set water pump
int pump_pin = 4;

// set button
int button_pin = 12;

// set water valve pins
int valve_pins[] = {6, 8, 9, 10};

// set all moisture sensors PIN ID
int moisture_pins[] = {A0, A1, A2, A3};

// declare moisture values
int moisture_values[] = {0, 0, 0, 0};

//valve states    1:open   0:close
int valve_state_flags[] = {0, 0, 0, 0};

//pump state    1:open   0:close
int pump_state_flag = 0;

// Water level
bool water_level_enabled = false;
VL53L0X water_level_sensor;
uint16_t water_level_per = 0;
uint16_t water_level_mm = 0;

// Values to help improve the capacitive sensor accuracy.
long moistureSensorFromLow = 590;
long moistureSensorToHigh = 290;

char days_of_the_week[7][12] = {
    "Sun",
    "Mon",
    "Tue",
    "Wed",
    "Thu",
    "Fri",
    "Sat",
};

// good flower
static const unsigned char bitmap_good[] U8X8_PROGMEM = {
    0x00, 0x42, 0x4C, 0x00, 0x00, 0xE6, 0x6E, 0x00, 0x00, 0xAE, 0x7B, 0x00, 0x00, 0x3A, 0x51, 0x00,
    0x00, 0x12, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x06, 0x40, 0x00, 0x00, 0x06, 0x40, 0x00,
    0x00, 0x04, 0x60, 0x00, 0x00, 0x0C, 0x20, 0x00, 0x00, 0x08, 0x30, 0x00, 0x00, 0x18, 0x18, 0x00,
    0x00, 0xE0, 0x0F, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0xC1, 0x00, 0x00, 0x0E, 0x61, 0x00,
    0x00, 0x1C, 0x79, 0x00, 0x00, 0x34, 0x29, 0x00, 0x00, 0x28, 0x35, 0x00, 0x00, 0x48, 0x17, 0x00,
    0x00, 0xD8, 0x1B, 0x00, 0x00, 0x90, 0x1B, 0x00, 0x00, 0xB0, 0x09, 0x00, 0x00, 0xA0, 0x05, 0x00,
    0x00, 0xE0, 0x07, 0x00, 0x00, 0xC0, 0x03, 0x00};

// bad flower
static const unsigned char bitmap_bad[] U8X8_PROGMEM = {
    0x00, 0x80, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0xE0, 0x0D, 0x00, 0x00, 0xA0, 0x0F, 0x00,
    0x00, 0x20, 0x69, 0x00, 0x00, 0x10, 0x78, 0x02, 0x00, 0x10, 0xC0, 0x03, 0x00, 0x10, 0xC0, 0x03,
    0x00, 0x10, 0x00, 0x01, 0x00, 0x10, 0x80, 0x00, 0x00, 0x10, 0xC0, 0x00, 0x00, 0x30, 0x60, 0x00,
    0x00, 0x60, 0x30, 0x00, 0x00, 0xC0, 0x1F, 0x00, 0x00, 0x60, 0x07, 0x00, 0x00, 0x60, 0x00, 0x00,
    0x00, 0x60, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xC7, 0x1C, 0x00,
    0x80, 0x68, 0x66, 0x00, 0xC0, 0x33, 0x7B, 0x00, 0x40, 0xB6, 0x4D, 0x00, 0x00, 0xE8, 0x06, 0x00,
    0x00, 0xF0, 0x03, 0x00, 0x00, 0xE0, 0x00, 0x00};

//  Logo
static const unsigned char bitmap_logo[] U8X8_PROGMEM = {
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00,
    0x18, 0x00, 0x00, 0x03, 0x0e, 0x00, 0xf8, 0xff, 0xc3, 0x00, 0x07, 0x0c,
    0x00, 0x00, 0x00, 0x18, 0x00, 0x80, 0x03, 0x0e, 0x30, 0xfc, 0xff, 0xc1,
    0x00, 0x07, 0x0e, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x80, 0x03, 0x0e, 0x30,
    0x80, 0x03, 0xe0, 0x00, 0x07, 0x0e, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x80,
    0x03, 0x06, 0x38, 0x80, 0x01, 0xe0, 0x00, 0x07, 0x0e, 0x00, 0x00, 0x00,
    0x1c, 0x00, 0x80, 0x01, 0x07, 0x38, 0xc0, 0x01, 0x60, 0x00, 0x07, 0x06,
    0x00, 0x00, 0x00, 0x1c, 0x00, 0x80, 0x01, 0x07, 0x3c, 0xc0, 0x01, 0x70,
    0x00, 0x07, 0x06, 0x00, 0x00, 0x03, 0x1c, 0x00, 0x80, 0x01, 0x03, 0x1c,
    0xc0, 0x00, 0x30, 0x00, 0x07, 0x06, 0x08, 0x80, 0x02, 0x0c, 0x00, 0xc0,
    0x01, 0x03, 0x1e, 0xc0, 0x00, 0x30, 0x80, 0x07, 0x07, 0x00, 0x40, 0x00,
    0x0c, 0x00, 0xc0, 0x81, 0x03, 0x1e, 0xe0, 0x00, 0x30, 0x80, 0x03, 0x07,
    0x00, 0x40, 0x00, 0x0c, 0x00, 0xc0, 0x81, 0x03, 0x1f, 0xe0, 0x00, 0x30,
    0xe0, 0x03, 0x07, 0x04, 0x40, 0x70, 0x0e, 0x00, 0xc0, 0x80, 0x83, 0x1b,
    0xe0, 0x00, 0x30, 0xf0, 0x03, 0x03, 0x24, 0x43, 0x88, 0x0e, 0x00, 0xc0,
    0x80, 0xc3, 0x19, 0xe0, 0x40, 0x30, 0x3c, 0x03, 0x03, 0xa4, 0xc4, 0x84,
    0x0e, 0xe0, 0xc7, 0x80, 0xe3, 0x18, 0xe0, 0x60, 0xf0, 0x1f, 0x03, 0x03,
    0xa4, 0x44, 0x84, 0xfe, 0xff, 0xc7, 0x00, 0x7f, 0x38, 0xe0, 0x78, 0xf0,
    0x87, 0x03, 0x03, 0x64, 0x44, 0x44, 0xff, 0xff, 0xc3, 0x00, 0x3f, 0x78,
    0xe0, 0x3f, 0xe0, 0x81, 0x03, 0x43, 0x24, 0x68, 0x38, 0x7f, 0x00, 0xc0,
    0x00, 0x0e, 0x70, 0xc0, 0x0f, 0x00, 0x80, 0x01, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xc0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xe0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c,
    0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x1c, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x1f, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x07, 0x00, 0x00, 0x00, 0x00, 0x00};

void setup()
{
  Wire.begin();
  RTC.begin();

  Serial.begin(19200);
  
#ifdef SEND_STATS_MQTT
  // Serial to ESP8266. Use RX & TX pins of Elecrow watering board
  Serial1.begin(19200);
  Serial.println("Started Serial1 for esp8266");
#endif

  u8g2.begin();
  // declare valve relays as output
  for (int i = 0; i < num_sensors; i++)
  {
    pinMode(valve_pins[i], OUTPUT);
  }
  // declare pump as output
  pinMode(pump_pin, OUTPUT);
  // declare switch as input
  pinMode(button_pin, INPUT);

  setup_water_level_sensor();

  u8g2.firstPage();
  do
  {
    draw_ad();
  } while (u8g2.nextPage());
}

void setup_water_level_sensor()
{
  water_level_enabled = false;
  water_level_sensor.setTimeout(500);
  if (water_level_sensor.init())
  {
    water_level_enabled = true;
    // High accuracy - increase timing budget to 200 ms
    water_level_sensor.setMeasurementTimingBudget(200000);
  }
}

void loop()
{
  // read the value from the moisture sensors:
  read_value();
  check_water_level();
  water_flower();
  send_stats();
  int button_state = digitalRead(button_pin);
  if (button_state == 1)
  {
    u8g2.firstPage();
    do
    {
      draw_stats();
      draw_flower();
    } while (u8g2.nextPage());
    delay(500);
  }
  else
  {
    u8g2.firstPage();
    do
    {
      draw_time();
      u8g2.drawStr(18, 55, "www.elecrow.com");
    } while (u8g2.nextPage());
    delay(500);
  }
  delay(500);
}

//Set moisture value
void read_value()
{
  /**************These is for resistor moisture sensor***********
  float value = analogRead(A0);
  moisture_values[i] = (value * 120) / 1023; delay(20);
  ...
 **********************************************************/
  /************These is for capacity moisture sensor*********/
  for (int i = 0; i < num_sensors; i++)
  {
    float value = analogRead(moisture_pins[i]);

    if (value > moistureSensorFromLow) {
      // Tune fromLow
      moistureSensorFromLow = value;
    }
    if (value < moistureSensorToHigh) {
      // Tune toHigh
      moistureSensorToHigh = value;
    }    
    moisture_values[i] = map(value, 590, 360, 0, 100);
    delay(20);
    if (moisture_values[i] < 0)
    {
      moisture_values[i] = 0;
    }
  }
}

void water_flower()
{
  for (int i = 0; i < num_sensors; i++)
  {
    if (moisture_values[i] < WATER_START_VALUE)
    {
      digitalWrite(valve_pins[i], HIGH);
      valve_state_flags[i] = 1;
      send_stats_force = true;
      delay(50);
      if (pump_state_flag == 0)
      {
        digitalWrite(pump_pin, HIGH);
        pump_state_flag = 1;
        delay(50);
      }
    }
    else if (moisture_values[i] > WATER_STOP_VALUE)
    {
      if (valve_state_flags[i] == 1) {
        // Only report if it IS on
        send_stats_force = true;
      }
      // Force it off
      digitalWrite(valve_pins[i], LOW);
      valve_state_flags[i] = 0;
      delay(50);
    }
  }

  // If no more active valves, shut down the pump.
  int num_active_valves = 0;
  for (int i = 0; i < num_sensors; i++)
  {
    num_active_valves += (valve_state_flags[i] > 0) ? 1 : 0;
  }
  if (num_active_valves == 0)
  {
    digitalWrite(pump_pin, LOW);
    pump_state_flag = 0;
    delay(50);
  }
}

void draw_ad()
{
  u8g2.setFont(u8g2_font_profont12_tr);
  u8g2.drawStr(5, 55, "www.elecrow.com");
  u8g2.drawXBMP(0, 0, 120, 34, bitmap_logo);
}

void draw_time()
{
  int x = 5;
  float i = 25.00;
  float j = 54;
  DateTime now = RTC.now();
  Serial.print(now.year(), DEC);
  if (!RTC.isrunning())
  {
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setCursor(5, 20);
    u8g2.print("RTC is NOT running!");
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  else
  {
    u8g2.setFont(u8g2_font_7x13_tr);

    char datestr[32]; //make this big enough to hold the resulting string
    snprintf(datestr, sizeof(datestr), "%4d-%02d-%02d  [%s]", now.year(), now.month(), now.day(), days_of_the_week[now.dayOfTheWeek()]);
    u8g2.setCursor(5, 11);
    u8g2.print(datestr);

    char timestr[9];
    snprintf(timestr, sizeof(timestr), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    u8g2.setCursor(35, 33);
    u8g2.print(timestr);
  }
}

//Style the flowers     bitmap_bad: bad flowers     bitmap_good:good  flowers
void draw_flower()
{
  for (int i = 0; i < num_sensors; i++)
  {
    if (moisture_values[i] < 30)
    {
      u8g2.drawXBMP(i * 32, 0, 32, 30, bitmap_bad);
    }
    else
    {
      u8g2.drawXBMP(i * 32, 0, 32, 30, bitmap_good);
    }
  }
}

void check_water_level()
{
  if (water_level_enabled)
  {
    // This will return WATER_LEVEL_TIMEOUT if a timeout happens.
    water_level_mm = water_level_sensor.readRangeSingleMillimeters();
    if (water_level_mm == WATER_LEVEL_TIMEOUT)
    {
      //Timeout reading the water level
      water_level_per = WATER_LEVEL_TIMEOUT;
    }
    else
    {
      if (water_level_mm > MAX_WATER_DEPTH)
      {
        water_level_mm = MAX_WATER_DEPTH;
      }
      water_level_mm = MAX_WATER_DEPTH - water_level_mm;
      water_level_per = (uint16_t) (((double) water_level_mm / (double) MAX_WATER_DEPTH) * 100);
    }
  }
}

void send_stats()
{
  unsigned long now = millis();
  unsigned long millisSinceLastRun = now - send_stats_last;
  static char output_buffer[10];
  
#ifdef SEND_STATS_MQTT
  dtostrf(millisSinceLastRun, 9, 0, output_buffer);
  Serial1.print("#Millis since last run ");
  Serial1.print(millisSinceLastRun);
  Serial1.print("\n");
#endif

  if (millisSinceLastRun > SEND_STATS_FREQ_MS) {
    // TODO: There is some issue that this isn't ever being triggered.
    send_stats_force = true;
  }
  if (send_stats_force)
  {
    send_stats_last = now;
    send_stats_force = false;
    Serial.println("Sendings stats\n");
    

    for (int i = 0; i < num_sensors; i++)
    {
      /*********Output Moisture Sensor values to ESP8266******/
      dtostrf(moisture_values[i], 4, 0, output_buffer);
#ifdef SEND_STATS_MQTT
      if (i != 0) {
        Serial1.print(",");
      }
      Serial1.print(output_buffer);
#endif
#ifdef SEND_STATS_LOCAL
      if (i != 0) {
        Serial.print(",");
      }
      Serial.print(output_buffer);
#endif
    }

    dtostrf(pump_state_flag, 1, 0, output_buffer);
#ifdef SEND_STATS_MQTT
    Serial1.print(",");
    Serial1.print(output_buffer);
#endif
#ifdef SEND_STATS_LOCAL
    Serial.print(",");
    Serial.print(output_buffer);
#endif

    dtostrf(water_level_mm, 4, 0, output_buffer);
#ifdef SEND_STATS_MQTT
    Serial1.print(",");
    Serial1.print(output_buffer);
#endif
#ifdef SEND_STATS_LOCAL
    Serial.print(",");
    Serial.print(output_buffer);
#endif

    dtostrf(water_level_per, 4, 0, output_buffer);
#ifdef SEND_STATS_MQTT
    Serial1.print(",");
    Serial1.print(output_buffer);
#endif
#ifdef SEND_STATS_LOCAL
    Serial.print(",");
    Serial.print(output_buffer);
#endif

    for (int i = 0; i < num_sensors; i++)
    {
      /*********Output Moisture Sensor values to ESP8266******/
      dtostrf(valve_state_flags[i], 1, 0, output_buffer);
#ifdef SEND_STATS_MQTT
      Serial1.print(",");
      Serial1.print(output_buffer);
#endif
#ifdef SEND_STATS_LOCAL
      Serial.print(",");
      Serial.print(output_buffer);
#endif
    }

    // End the message.
#ifdef SEND_STATS_MQTT
    Serial1.print("\n");
#endif
#ifdef SEND_STATS_LOCAL
    Serial.print("\n");
#endif
  }
}

void draw_stats()
{
  int x_offsets[] = {0, 32, 64, 96};
  char display_buffer[5] = {0};

  u8g2.setFont(u8g2_font_8x13_tr);
  u8g2.setCursor(9, 60);
  u8g2.print("W. LEVEL");
  if (!water_level_enabled)
  {
    u8g2.drawStr(x_offsets[2] + 16, 60, "N/C");
  }
  else if (water_level_per == WATER_LEVEL_TIMEOUT)
  {
    u8g2.drawStr(x_offsets[2] + 16, 60, "T/O");
  }
  else
  {
    itoa(water_level_per, display_buffer, 10);
    // itoa(water_level_mm, display_buffer, 10);
    u8g2.drawStr(x_offsets[2] + 16, 60, display_buffer);
  }

  for (int i = 0; i < num_sensors; i++)
  {
    itoa(moisture_values[i], display_buffer, 10);
    if (moisture_values[i] < 10)
    {
      u8g2.drawStr(x_offsets[i] + 14, 45, display_buffer);
    }
    else if (moisture_values[i] < 100)
    {
      u8g2.drawStr(x_offsets[i] + 5, 45, display_buffer);
    }
    else
    {
      moisture_values[i] = 100;
      itoa(moisture_values[i], display_buffer, 10);
      u8g2.drawStr(x_offsets[i] + 2, 45, display_buffer);
    }
    u8g2.setCursor(x_offsets[i] + 23, 45);
    u8g2.print("%");
  }
}
