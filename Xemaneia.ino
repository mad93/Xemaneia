#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define OK_LED 8
#define KO_LED 9

#define BOMBA 7

// DS18S20 Temperature chip i/o
OneWire ds(10);  // on pin 10

// Bomb state
bool bombActive = false;

// Protect against wrong height 
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup(void) {
  // initialize inputs/outputs
  // start serial port
  Serial.begin(9600);

  // Initialize display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.display();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.clearDisplay();

  // Status Leds
  // Signaling ok
  pinMode(OK_LED, OUTPUT);
  // Signaling ko
  pinMode(KO_LED, OUTPUT);

  // Relé
  pinMode(BOMBA, OUTPUT);
  digitalWrite(BOMBA, LOW);
}

void setStatusLed(bool ok)
{
  if(ok)
  {
    digitalWrite(OK_LED, HIGH);
    digitalWrite(KO_LED, LOW);
  } else {
    digitalWrite(OK_LED, LOW);
    digitalWrite(KO_LED, HIGH);
  }
}

void disableBomb(bool *bombActive)
{
  *bombActive = false;
  digitalWrite(BOMBA, HIGH);
  Serial.print("Bomb Disabled.\n");
}

void enableBomb(bool *bombActive)
{
  *bombActive = true;
  digitalWrite(BOMBA, LOW);
  Serial.print("Bomb Enabled.\n");
}

void loop(void) {
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8];
  int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;

  display.clearDisplay();
  ds.reset_search();
  if ( !ds.search(addr)) {
      Serial.print("No more addresses.\n");
      setStatusLed(false);
      display.setCursor(0,0);
      display.setTextSize(1);
      display.println("RieraCal");
      display.println("Xemeneia v0.1");
      display.setCursor(0,24);
      display.setTextSize(3);
      display.println("ERROR");
      display.setTextSize(1);
      display.setCursor(0,56);
      display.println("No sensor found!");
      display.display();
      ds.reset_search();
      disableBomb(&bombActive);
      return;
  }

  Serial.print("R=");
  for( i = 0; i < 8; i++) {
    Serial.print(addr[i], HEX);
    Serial.print(" ");
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print("CRC is not valid!\n");
      disableBomb(&bombActive);
      return;
  }

  if ( addr[0] == 0x10) {
      Serial.print("Device is a DS18S20 family device.\n");
  }
  else if ( addr[0] == 0x28) {
      Serial.print("Device is a DS18B20 family device.\n");
  }
  else {
      Serial.print("Device family is not recognized: 0x");
      Serial.println(addr[0],HEX);
      disableBomb(&bombActive);
      return;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("P=");
  Serial.print(present,HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print( OneWire::crc8( data, 8), HEX);
  Serial.println();

  LowByte = data[0];
  HighByte = data[1];
  TReading = (HighByte << 8) + LowByte;
  SignBit = TReading & 0x8000;  // test most sig bit
  if (SignBit) // negative
  {
    TReading = (TReading ^ 0xffff) + 1; // 2's comp
  }
  Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25

  Whole = Tc_100 / 100;  // separate off the whole and fractional portions
  Fract = Tc_100 % 100;


  if (SignBit) // If its negative
  {
     Serial.print("-");
  }
  Serial.print(Whole);
  Serial.print(".");
  if (Fract < 10)
  {
     Serial.print("0");
  }
  Serial.print(Fract);
  Serial.print("\n");

  if(!SignBit && Whole >= 90)
    enableBomb(&bombActive);
  else
  {
    if(!bombActive)
    {
      disableBomb(&bombActive);
      Serial.println("First if");
    }
    else if(bombActive && (SignBit || Whole < 70))
    {
      disableBomb(&bombActive);
      Serial.println("Second if");
    }
    else
    {
      enableBomb(&bombActive);
      Serial.println("Third if");
    }
    
  }
  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("RieraCal");
  display.println("Xemeneia v0.1");
  display.setCursor(0,24);
  display.setTextSize(3);
  display.print(Whole);
  display.print(",");
  display.print(Fract);
  display.println("C");
  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,56);

  if(bombActive)
    display.println("Estat bomba: ACTIVA");
  else
    display.println("Estat bomba: INACTIVA");
    
  display.display();
  setStatusLed(true);
}

