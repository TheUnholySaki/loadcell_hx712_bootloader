/*
 * @brief Send data to Slave
 * @param data : send 1 byte data to slave
 */
void send_data_i2c(int data) {
  Wire.beginTransmission(SLAVE_ADDRESS); // transmit to device #8
  Wire.write(data);              // sends one byte
  Wire.endTransmission();    // stop transmitting
}

/*
 * @brief Read data from Slave
 * @retval message in int format
 */
int read_data_i2c(){
  Wire.requestFrom(SLAVE_ADDRESS, 4);
  long temp = 0;
  int ack = 0;
  while (Wire.available()) {
      ack = 1;
      char c = Wire.read();
      if (c < 48 || c > 57); 
      else {
        temp = temp * 10 + ((int)c - 48); // 48
      }
  }
  return temp;
}

/*
 * @brief verify checksum algorithm
 * @retval true for pass, false for not pass
 */
bool check_cs(int cs, int line_length) {
  int total_cs = 0;
  for (int i = 0; i < line_length; i++) {
    total_cs += data[i];
  }
  int calc_cs = (256 - total_cs % 256) % 256;
  if (calc_cs == cs) return true;
  else return false;
}
