#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#include <DS1302.h>
//Temperature Sensor BME280 x2
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI
// Initialize I2C buses using TCA9548A I2C Multiplexer
    #define TCAADDR 0x70
     
    void tcaselect(uint8_t i) {
      if (i > 7) return;
     
      Wire.beginTransmission(TCAADDR);
      Wire.write(1 << i);
      Wire.endTransmission();  
    }

    double temp, temp1, temp2;
    int hum, hum1, hum2, pres, pres1, pres2;

byte degree[8] = //degree symbol code
{
B00111,
B00101,
B00111,
B00000,
B00000,
B00000,
B00000,
};
DS1302 rtc(10, 9, 8);       //time module, pins: RST,DAT,CLK 

//initialize screen, Arduino pins order:
// RS, E, DB4, DB5, DB6, DB7
LiquidCrystal_I2C lcd(0x27,20,4);  //set display
float PhVal=0.0;
bool light=false;
int TankValue;
#define VREF 5.0      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0;
unsigned long timing=2000; //request rate to temp sensors

void setup() {
  Wire.begin();
  Serial.begin(9600);
  tcaselect(0);
  delay(100);
  lcd.init();                     
  lcd.backlight();//turn on display light
  lcd.createChar(1, degree); //create symbol #1
  lcd.setCursor(0, 0);   
  lcd.print("     @Kuhnyafarm");
  delay(2000);
  lcd.setCursor(0, 1);   
  lcd.print("Temp Sensor");
  lcd.setCursor(0, 2);
  lcd.print("Ph Sensor");
  lcd.setCursor(0, 3);
  lcd.print("TDS Sensor");
  delay(1000);
  lcd.setCursor(11, 1);
  bool status;
    tcaselect(1);
    delay(100);
    // default settings
    status = bme.begin();
    tcaselect(0);
      delay(100);
    if (!status) {
  lcd.print(".....ERROR");
    }
  else 
  {
  lcd.print(".....OK");
  }    
  delay(1000);
  
  lcd.setCursor(9, 2);
  if (analogRead(3)>700.0 and analogRead(3)<800.0) //check Ph sensor
  {
  lcd.print(".....OK");
  }
  else 
  {
  lcd.print(".....ERROR");
  }
  delay(1000);
  
  lcd.setCursor(10, 3);
  if (analogRead(2)>500.0 and analogRead(2)<600.0) //check TDS sensor
  {
  lcd.print("....ERROR");
  }
  else 
  {
  lcd.print("....OK");
  }
  delay(1000);
  
  // Πελε 12V
  pinMode(22, INPUT);  // relay off lamp cooler 3 floor in2
  pinMode(24, INPUT);  // relay off lamp cooler 4 floor in3
  pinMode(26, INPUT);  // relay off lamp cooler 2 floor in8
  pinMode(28, INPUT);  // relay off cooler main in5
  pinMode(30, INPUT);  // relay off pump Ph Up in6
  pinMode(32, INPUT);  // relay off pump Ph Down in7
  pinMode(34, INPUT);  // relay off pump Ph control in1
  pinMode(36, INPUT);  // relay off cooler humidifier in4
  // Πελε 220V
  pinMode(23, INPUT);  // empty in2
  pinMode(25, INPUT);  // empty in3
  pinMode(27, INPUT);  // empty in4
  pinMode(29, INPUT);  // empty in5
  pinMode(31, INPUT);  // empty in6
  pinMode(33, INPUT);  // relay off pump in7
  pinMode(35, INPUT);  // relay off humidifier in8
  pinMode(37, INPUT);  // relay off lamps 2,3,4 floor in1
  lcd.clear();
}

void loop() {
  
  // Time
  Time t = rtc.time();  
  t.yr=t.yr-2000;
  
  // Tank Level Sensor
  TankValue = analogRead(A0);
  TankValue = map(TankValue, 300, 520, 0, 100);
  if (TankValue>100)
    {
    TankValue=100;
    }
  if (TankValue<0)
    {
    TankValue=0;
    }
  
  // Tempeature Sensor
  tcaselect(4);
  delay(100);
  temp=bme.readTemperature();
  pres=bme.readPressure() / 100 * 0.75;
  hum=bme.readHumidity();
  tcaselect(1);
  delay(100);
  temp2=bme.readTemperature();
  pres2=bme.readPressure() / 100 * 0.75;
  hum2=bme.readHumidity();
  delay(100);
  temp=temp1+temp2;
  pres=pres1+pres2;
  hum=hum1+hum2;
  tcaselect(0);
  
  
  // Ph Sensor
  for(int i=0;i<20;i++)
  {
   float PV=analogRead(A3);
   PhVal=PhVal+PV;
   delay(250);
  }
  PhVal=PhVal/20*5.0/1023;
  double pH = (5.0-PhVal)*1000/59.16/4;
  PhVal=0.0;

  // TDS Sensor
  static unsigned long analogSampleTimepoint = millis();
   if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
   {
     analogSampleTimepoint = millis();
     analogBuffer[analogBufferIndex] = analogRead(A2);    //read the analog value and store into the buffer
     analogBufferIndex++;
     if(analogBufferIndex == SCOUNT) 
         analogBufferIndex = 0;
   }   
   static unsigned long printTimepoint = millis();
   if(millis()-printTimepoint > 800U)
   {
      printTimepoint = millis();
      for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
        analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1023.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(temp-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage*compensationCoefficient;  //temperature compensation
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
   }
  
  // Display
  lcd.setCursor(0, 0);          // set cursor β col 0, row 0
      if (t.date<10) {
 lcd.print("0");
  }
  lcd.print(t.date);lcd.print("."); 
    if (t.mon<10) {
 lcd.print("0");
  }
  lcd.print(t.mon); lcd.print("."); lcd.print(t.yr);         // print first string
  lcd.print("  "); 
      if (t.hr<10) {
 lcd.print("0");
  }
  lcd.print(t.hr); lcd.print(":"); 
      if (t.min<10) {
 lcd.print("0");
  }
  lcd.print(t.min); 
  lcd.setCursor(0, 1);            // set cursor β col 0, row 1
  
  lcd.print("H: "); lcd.print(hum); // print second string
  if (hum==100)
    {
    lcd.print("%"); 
    }
    else
    {
     lcd.print(" %");
    }
    lcd.print("   T: "); lcd.print(temp,1);  lcd.print("\1""C");   
  
  lcd.setCursor(0, 2);            // set cursor β col 0, row 2
      if (isnan(pH) || pH==0)           // check readings
      {
       lcd.print("Ph: Error");
      }
  else 
  {
    if (digitalRead(15)==HIGH)
    {
      lcd.setCursor(0, 2);
      lcd.print("         ");
      lcd.setCursor(0, 2);
      lcd.print("Ph: ");
      lcd.print(pH,1);
    }
  }
    if (digitalRead(16)==HIGH and digitalRead(15)==LOW)
    {
      lcd.setCursor(0, 2);
      lcd.print("         ");
      lcd.setCursor(0, 2);
      lcd.print("TDS: ");
      lcd.print(tdsValue,0);
    }
    if (digitalRead(15)==LOW and digitalRead(16)==LOW)
    {
      lcd.setCursor(0, 2);
      lcd.print("         ");
      lcd.setCursor(0, 2);
      lcd.print("Ph: Off");
    }
  lcd.setCursor(10, 2);  
  lcd.print("Lvl: "); 			// print third string
  lcd.setCursor(15, 2); lcd.print("     ");
  lcd.setCursor(15, 2);
  lcd.print(TankValue); lcd.print(" %");
 
  lcd.setCursor(0, 3);
  lcd.print("Pr: ");
  lcd.print(pres);
  lcd.setCursor(10, 3);
  lcd.print("Sys: ");
  lcd.print("   ");
  lcd.setCursor(15, 3);
  if ((digitalRead(15)==HIGH and (pH>=6.5 or pH<=5.2)) or (digitalRead(16)==HIGH and tdsValue>1500) or TankValue<=20 or temp<=20 or temp>28)
  {
  lcd.print("(!)  ");
  }
  else
  {
  lcd.print("OK   ");
  }


  // RELAY //

  
  // Temperature
  if (temp>=25.0 or (t.hr>=8 and t.hr<23 and t.min>=30 and t.min<35))
  {
    pinMode(28, OUTPUT);   // relay main coller on in5
  }
  if (temp<=24.0 and (t.min<30 or t.min>=35))
  {
    pinMode(28, INPUT);   // relay main coller off in5
  }

  // Main pump
  if((t.hr==8 or t.hr==12 or t.hr==16 or t.hr==20) and t.min<5 and TankValue>=5)
  {
    pinMode(33, OUTPUT);  // relay pump on in7
    delay(100);
  }
  else
  {
    pinMode(33, INPUT);  // relay pump off in7
    delay(100);
  }
  
  // Lamps and coolers
  if (t.hr>=8 and t.hr<22)
  {
    // 12V
  pinMode(22, OUTPUT);  // relay on cooler lamp 3 floor in2
  pinMode(24, OUTPUT);  // relay on cooler lamp 4 floor in3
  pinMode(26, OUTPUT);  // relay on cooler lamp 2 floor in8
    // 220V
  pinMode(37, OUTPUT);  // relay on cooler lamp 2,3,4 floors in1
  }
  if (t.hr<8 or t.hr>=22)
  {
    // 12V
  pinMode(22, INPUT);  // relay off cooler lamp 3 floor in2
  pinMode(24, INPUT);  // relay off cooler lamp 4 floor in3
  pinMode(26, INPUT);  // relay off cooler lamp 2 floor in8
  pinMode(36, INPUT);  // relay off cooler humidifier in4
    // 220V
  pinMode(35, INPUT);  // relay off humidifier in8
  pinMode(37, INPUT);  // relay off cooler lamps 2,3,4 floors in1
  }

  // Humidifier
  if (t.hr>=8 and t.hr<23 and hum<=55)
  {
    // 12V
    pinMode(36, OUTPUT);  // relay on cooler humidifier in4
    // 24V
    pinMode(35, OUTPUT);  // relay on humidifier in8
  }
  if (t.hr<8 or t.hr==23 or hum>=65)
  {
    // 12V
    pinMode(36, INPUT);  // relay off cooler humidifier in4
    // 24V
    pinMode(35, INPUT);  // relay off humidifier in8
  }

  // Ph pump control
  if((t.hr==9 or t.hr==13 or t.hr==15 or t.hr==17 or t.hr==19 or t.hr==21) and ((t.min>=0 and t.min<1) or (t.min>=30 and t.min<31)) and digitalRead(15)==HIGH)
  {
  pinMode(34, OUTPUT);  // relay on pump Ph control in1
  }
  else
  {
  pinMode(34, INPUT);  // relay off pump Ph control in1
  }
  
  // Pump Ph UP
  int timer=1200*TankValue/100;
  if((t.hr==9 or t.hr==13 or t.hr==15 or t.hr==17 or t.hr==19 or t.hr==21) and (t.min==15 or t.min==45) and pH<5.5 and digitalRead(15)==HIGH)
  {
  pinMode(30, OUTPUT);  // relay on pump Ph Up in6
  delay(timer);
  pinMode(30, INPUT);  // relay off pump Ph Up in6
  lcd.setCursor(15, 3);
  lcd.print("    ");
  lcd.setCursor(15, 3);
  lcd.print("Ph Up");
  delay(80000);
  }
  // Pump Ph DOWN
  if((t.hr==9 or t.hr==13 or t.hr==15 or t.hr==17 or t.hr==19 or t.hr==21) and (t.min==15 or t.min==45) and pH>6.3 and digitalRead(15)==HIGH)
  {
  pinMode(32, OUTPUT);  // relay on pump Ph Down in7 
  delay(timer);
  pinMode(32, INPUT);  // relay off pump Ph Down in7
  lcd.setCursor(15, 3);
  lcd.print("    ");
  lcd.setCursor(15, 3);
  lcd.print("Ph Dn");
  delay(80000);
  }
  
}

int getMedianNum(int bArray[], int iFilterLen) 
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
    bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++) 
      {
    for (i = 0; i < iFilterLen - j - 1; i++) 
          {
      if (bTab[i] > bTab[i + 1]) 
            {
    bTemp = bTab[i];
          bTab[i] = bTab[i + 1];
    bTab[i + 1] = bTemp;
       }
    }
      }
      if ((iFilterLen & 1) > 0)
  bTemp = bTab[(iFilterLen - 1) / 2];
      else
  bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;

}