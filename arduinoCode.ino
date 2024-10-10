#include <Servo.h>
#include <DHT.h>
#include <LiquidCrystal.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#define DHTPIN 2

#define SERVOPIN 11

#define DHTTYPE DHT11   // DHT 11 actual sensor in physikal system
//#define DHTTYPE DHT22 // dht22 only one available in wokwi

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 1  // Anzahl der 8x8-Matrizen
#define CLK_PIN   8  // Clock
#define DATA_PIN  10  // Data in (DIN)
#define CS_PIN    9  // Chip Select (CS)

DHT dht(DHTPIN, DHTTYPE); // temp sensor initialisation

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN); // matrix initialisation

Servo servo; // object servo of type Servo
float const dPos = 0; // default position
float cPos = 0; // position helper
float step = 0.25; // "speed"

bool bState; //helper for button state
int button = 12; // button on pin 12

LiquidCrystal lcd(7, 6, 5, 4, 3, 13);  // RS, E, D4, D5, D6, D7
unsigned long acStartTime = 0;
unsigned long totalAcOnTime = 0;
unsigned long currentAcOnTime = 0;
float totalPowerConsumption = 0;  // in Watt-hours
bool acOn = false;

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(button, INPUT_PULLUP);
  servo.attach(SERVOPIN); // servo on pin 11
  servo.write(dPos); // put arm in default position
  
  lcd.begin(16, 2);

  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 8);  // brightness 0-15
  mx.clear();

}

void displayH() {
  byte H[8] = {
    B10000010,  // column 1
    B10000010,  // column 2
    B10000010,  // column 3
    B11111110,  // column 4
    B10000010,  // column 5
    B10000010,  // column 6
    B10000010,  // column 7
    B00000000   // column 8
  };

  for (int i = 0; i < 8; i++) {
    mx.setRow(0, i, H[i]);  // iterating through matrix grid and byte

  }
}

void displayO() {
  byte O[8] = {
    B00000000,  // column 1
    B00000000,  // column 2
    B00000000,  // column 3
    B00000000,  // column 4
    B00000000,  // column 5
    B00000000,  // column 6
    B00000000,  // column 7
    B00000000   // column 8
  };

  for (int i = 0; i < 8; i++) {
    mx.setRow(0, i, O[i]);  // iterating through matrix grid and byte

  }
}

void displayK() {
  byte K[8] = {
    B01000001,  // column 1
    B00100001,  // column 2
    B00010001,  // column 3
    B00001001,  // column 4
    B00001101,  // column 5
    B00010011,  // column 6
    B00100001,  // column 7
    B01000001   // column 8
  };

  for (int i = 0; i < 8; i++) {
    mx.setRow(0, i, K[i]);  // iterating through matrix grid and byte

  }
}

float calculatePowerConsumption(unsigned long duration) {
  // Average bus-AC uses 32000 watts
  return (27000.0 * duration) / (1000.0 * 3600.0);  // Convert to Watt-hours

}

void printTime(unsigned long timeInSeconds) {
  int hours = timeInSeconds / 3600;
  int minutes = (timeInSeconds % 3600) / 60;
  int seconds = timeInSeconds % 60;

  if (hours > 0) {
    lcd.print(hours);
    lcd.print("h");
  }
  if (minutes > 0 || hours > 0) {
    lcd.print(minutes);
    lcd.print("m");
  }
  lcd.print(seconds);
  lcd.print("s");

}

void updateLCD(float temperature) {
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature, 1);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("AC:");
  printTime(totalAcOnTime + currentAcOnTime);
  lcd.print(" ");
  lcd.print(totalPowerConsumption / 1000.0, 2); // Convert to kWh, display with 2 decimal places
  lcd.print("kWh");

}

void windowClose()
{
  while(cPos > 0)
  {
    cPos -= step;
    servo.write(cPos);
    delay(10); // servo travel-time

  }
}

void windowOpen()
{
  while(cPos < 45)
  {
    cPos += step;
    servo.write(cPos);
    delay(10);

  }
}

void sendDataToPi(float temperature, float humidity, unsigned long acRunningTime, float powerConsumption) {
  Serial.print("Temperature: ");
  Serial.print(temperature, 2);
  Serial.print(", Humidity: ");
  Serial.print(humidity, 2);
  Serial.print(", AC Running Time: ");
  Serial.print(acRunningTime);  // This should not include 'seconds'
  Serial.print(", Power Consumption: ");
  Serial.print(powerConsumption / 1000.0, 2); // Convert to kWh
  Serial.println(" kWh");
}

unsigned long lastDataSentTime = 0;  // Time of the last data sent
const unsigned long dataInterval = 10000;  // 10 seconds interval for sending data

unsigned long lastLCDUpdateTime = 0;  // Time of the last LCD update
const unsigned long lcdUpdateInterval = 1000;  // 1 second interval for updating LCD

void loop() {
  float h = dht.readHumidity(); // Read humidity
  float t = dht.readTemperature(); // Read temp in Celsius

  if(!digitalRead(button) && bState == false) // button pressed & current state is false
  {
    bState = true;

  } else if(!digitalRead(button) && bState == true) // button pressed & current state is true
  {
    bState = false;

  }
  
  if(bState == false && acOn == false) // button state toggled to false = move to position 0
  {
    windowClose();

  } else if(acOn == false) // button state toggled to true = move to position 180
  {
    windowOpen();

  }

  if(t > 25) // if temp over 22C
  {
    if(cPos > 0)
    {  
      windowClose();
      bState = false; // set button state to "windows closed"

    }
    
    if(!acOn)
    {
      acStartTime = millis();
    
    }

    acOn = true;
    displayK();

  } else if(t < 20) // if temp under 20C
  {
    if(cPos > 0)
    {
      windowClose();
      bState = false;
    
    }
    
    if(!acOn)
    {
      acStartTime = millis();
    
    }
    
    acOn = true;
    displayH();

  } else
  {
    if(acOn)
    {
      totalAcOnTime += currentAcOnTime;
    }
    
    acOn = false;
    displayO();

  }
  
  if (acOn)
  {
    currentAcOnTime = (millis() - acStartTime) / 1000;  // Convert to seconds and update time running
    totalPowerConsumption += calculatePowerConsumption(millis() - acStartTime); //update total power cons.

  }
  //updateLCD(t);

   // Check if it's time to update the LCD (every 1 second)
  if (millis() - lastLCDUpdateTime >= lcdUpdateInterval) {
    updateLCD(t);  // Update the LCD with the latest temperature, AC runtime, and power consumption
    lastLCDUpdateTime = millis();  // Update the last LCD update time
  }

  // // Send the data over Serial to Raspberry Pi
  // Serial.print(t);                      // Send temperature
  // Serial.print(","); 
  // Serial.print(currentAcOnTime);         // Send AC running time
  // Serial.print(",");
  // Serial.println(totalPowerConsumption); // Send total power consumption

 // Sending data to Raspberry Pi
 //  sendDataToPi(t, h, totalAcOnTime, totalPowerConsumption);

  // Check if 10 seconds have passed since the last data was sent
  if (millis() - lastDataSentTime >= dataInterval) {
    sendDataToPi(t, h, totalAcOnTime, totalPowerConsumption);  // Send data to Raspberry Pi
    lastDataSentTime = millis();  // Update the last data sent time
  }

  delay(20);  // Delay for 1 second before next iteration

}