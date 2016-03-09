// Distributed with a free-will license.
// Use it any way you want, profit or free, provided it fits in the licenses of its associated works.
// SHT31
// This code is designed to work with the SHT31_I2CS I2C Mini Module available from ControlEverything.com.
// https://www.controleverything.com/content/Humidity?sku=SHT31_I2CS#tabs-0-product_tabset-2

#include <wire.h>

// SHT31 I2C address is 0x44(68)
#define Addr 0x44

void setup() 
{
  // Initialise I2C communication as MASTER 
  Wire.begin();
  // Initialise serial communication, set baud rate = 9600
  Serial.begin(9600);
  
  // Begin transmission with given device on I2C bus
  Wire.beginTransmission(Addr);
  // Send 16-bit command byte          
  Wire.write(0x2C);
  Wire.write(0x06);
  // Stop I2C transmission on the device
  Wire.endTransmission();
  delay(300);
}

void loop() 
{   
  int data[6];
  // Start I2C Transmission on the device
  Wire.beginTransmission(Addr);
  // Stop I2C Transmission on the device
  Wire.endTransmission();
  
  // Request 6 bytes of data from the device
  Wire.requestFrom(Addr, 6);

  // Read 6 bytes of data
  // temp msb, temp lsb, crc, hum msb, hum lsb, crc
  if(Wire.available() == 6)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
    data[2] = Wire.read();
    data[3] = Wire.read();
    data[4] = Wire.read();
    data[5] = Wire.read();
  }
  delay(300);
  
  // Convert the data
  float cTemp = (((data[0] & 0xFF) * 256 + (data[1] & 0xFF)) * 175.72) / 65536 - 46.85;
  float fTemp = (cTemp * 1.8) + 32;
  float humidity = (125 * ((data[3] & 0xFF) * 256 + (data[4] & 0xFF))) / 65535.0 - 6;
    
        
  // Output data to serial monitor
  Serial.print("Temperature in celcius  : ");
  Serial.println(cTemp);
  Serial.print("Temperature in Fahrenheit  : ");
  Serial.println(fTemp);
  Serial.print("Relative Humidity    :  ");
  Serial.println(humidity);
}

