/*
 * @brief call function read weight and output
 * measurement to Serial COM
 */
void weight_scale_read() {
  digitalWrite(LED_BUILTIN, HIGH);  // LED indicator for normal mode
  weight = int_to_gram();
  Serial.print(weight*100, 0);
}

/*
 * @brief Calibrate measurement from ADC
 * @retval weight in gram
 */
double int_to_gram() {
  int32_t total = 0;
  int8_t samples = 2;
  long wo_load_coef = 7718930;
  float coef= -0.00131;
  for (uint16_t i = 0; i < samples; i++) {
    total += get_value();
  }
  double average = (float)(total / samples);
  double milligram = (average - wo_load_coef) * coef;
  return milligram;
}

/*
 * @brief request data from Slave
 * Convert string type to int
 * @retval measurement in int
 */
int32_t get_value() {
  Wire.requestFrom(SLAVE_ADDRESS, 10);  // request 6 bytes from slave device #8
  long temp = 0;
  uint8_t sign;

  while (Wire.available()) {  // slave may send less than requested
    char c = Wire.read();     // receive a byte as character

    //checking proper number or not
    //48 = "0", 57 ="9", 46 = ".", 45 = "-"
    if ((c < 48 || c > 57) && c != 46 && c != 45) {
      // if c is not appropriated, call this
    } else {
      // char_to_int
      if (counter == 0) {        // check negative sign
        if (c == 45) sign = -1;  // if there is "-", set
        else sign = 1;
        counter = 1;
        temp = 0;  // reset temp
      }
      temp = temp * 10 + ((int)c - 48);
    }

    if (!Wire.available()) {
      //Serial.print("\r\n");  //print end of line
      counter = 0;
      break;
    }
  }

  delay(100 / 4); // default rate = 10 Hz -> read at 5Hz
  return temp;
}