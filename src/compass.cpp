#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <QMC5883LCompass.h>

QMC5883LCompass compass;

void comapass_setup(void) 
{
  //Serial.begin(9600);
  Serial.println("HMC5883 Magnetometer Test"); Serial.println("");
  compass.init();

}

void compass_loop(void) 
{
  compass.read();

  byte a = compass.getAzimuth();
  // Output here will be a value from 0 - 15 based on the direction of the bearing / azimuth.
  byte b = compass.getBearing(a);
  
  Serial.print("B: ");
  Serial.print(b);
  Serial.println();
  
  delay(250);
}