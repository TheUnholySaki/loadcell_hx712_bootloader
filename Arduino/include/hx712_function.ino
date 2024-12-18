#include "Arduino.h"
#include <Wire.h>

/// @brief request data from Slave
/// Convert string type to int
/// @param adr Slave address to request data
/// @return (int) nearsurement
int32_t get_value(uint8_t adr)
{
  long temp = 0;
  uint8_t sign = 0;
  int counter = 0;
  Wire.requestFrom(adr, (uint8_t)10); // request 10 bytes from slave device with address adr
  while (Wire.available())
  {                       // slave may send less than requested
    char c = Wire.read(); // receive a byte as character

    // checking proper number or not
    if ((c < 48 || c > 57) && c != 46 && c != 45) // 48 = "0", 57 ="9", 46 = ".", 45 = "-"
    {
      // if c is not appropriated, call this
    }
    else
    {
      // char_to_int
      if (counter == 0)
      { // check negative sign
        if (c == 45)
          sign = -1; // if there is "-", set
        else
          sign = 1;
        counter = 1;
        temp = 0; // reset temp
      }
      temp = sign * (temp * 10 + ((int)c - 48));
    }
  }

  delay(100 / 4); // default rate = 10 Hz -> read at 5Hz
  return temp;
}

/// @brief Calibrate measurement from ADC
/// @param adr Slave address to request data
/// @return weight in gram
double weight_calib(uint8_t adr)
{
  int32_t total = 0;
  uint8_t samples = 2;
  long offset = 7718930;
  float coef = -0.00131;
  for (uint8_t i = 0; i < samples; i++)
  {
    total += get_value(adr);
  }
  double avg = (float)(total / samples);
  double value = (avg - offset) * coef;
  return value;
}

/// @brief call function read weight and output
/// measurement to Serial COM
/// @param adr Slave address to request data
void weight_scale_read(uint8_t adr)
{
  double weight = weight_calib(adr);
  Serial.print(weight * 100, 0);
}
