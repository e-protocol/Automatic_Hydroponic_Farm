#include <Wire.h>
#include <LiquidCrystal_I2C.h>
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
 
// инициализируем объект-экран, передаём использованные
// для подключения контакты на Arduino в порядке:
// RS, E, DB4, DB5, DB6, DB7
LiquidCrystal_I2C lcd(0x3F,20,4);  // Устанавливаем дисплей


// Initialize I2C buses using TCA9548A I2C Multiplexer
    #define TCAADDR 0x70
     
    void tcaselect(uint8_t i) {
      if (i > 7) return;
     
      Wire.beginTransmission(TCAADDR);
      Wire.write(1 << i);
      Wire.endTransmission();  
    }

    double temp, temp1, temp2, pres, pres1, pres2, hum, hum1, hum2;

void setup(){
  Wire.begin();
    Serial.begin(115200);
    tcaselect(0);
    lcd.init();                     
  lcd.backlight();// Включаем подсветку дисплея  
  delay(100);
  
bool status;
    tcaselect(2);
    delay(100);
    // default settings
    status = bme.begin();
    if (!status) {
      tcaselect(0);
      lcd.setCursor(0, 0);   
  lcd.print("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }   
    delay(100); // let sensor boot up
    lcd.clear();
} 
 
void loop(){
  
  tcaselect(2);
  temp1=bme.readTemperature();
  pres1=bme.readPressure();
  hum1=bme.readHumidity();
  delay(100);
  tcaselect(3);
  temp2=bme.readTemperature();
  pres2=bme.readPressure();
  hum2=bme.readHumidity();
  delay(100);
  temp=(temp1+temp2)/2;
  pres=(pres1+pres2)/2;
  hum=(hum1+hum2)/2;
  tcaselect(0);
  
  lcd.setCursor(0, 0);   
  lcd.print("Temp=");
    lcd.print(temp);
    lcd.print(" *C");

 lcd.setCursor(0, 1); 
    lcd.print("Pressure=");
 
    lcd.print(pres / 100 * 0.75);
    lcd.print(" mm");



 lcd.setCursor(0, 2); 
    lcd.print("Humidity = ");
    lcd.print(hum);
    lcd.print(" %");
    delay(1000);
}

