#include "Arduino.h"
#include <Wire.h>
#include "bootloader_function.ino"
#include "hx712_function.ino"

#define READ_BYTECOUNT 1
#define READ_ADDRESS 2
#define READ_RECORDTYPE 3
#define READ_DATAPART 4
#define READ_CHECKSUM 5
#define READ_END 6

#define WEIGHT_SCALE_MODE 0xF1
#define UPLOADING_MODE 0xF2
#define ERASE_MODE 0xF3
#define SYSRESET_MODE 0xF4
#define GO2UPAPP_MODE 0xF5
#define UPDMAIN_MODE 0xF6

#define SLAVE_ADDRESS 8

bool is_mode_selected = false;
bool pc_rq_weight_scale = false;
bool pc_rq_upload = false;
bool pc_rq_erase_mem = false;
bool pc_rq_sysreset = false;
bool pc_rq_go2UpApp = false;
bool pc_rq_updmain = false;

// VARIABLE FOR BOOTLOADER
uint8_t state;
bool is_pass_cs = false;
int data[100];
int InBytes;
int i = 0;
uint8_t line_length = 0;
// =============CODE=============

void setup()
{
  Wire.begin();                    // join I2C bus (address optional for master)
  Serial.begin(76800, SERIAL_8N1); // start serial for output 234000/4
  state = READ_BYTECOUNT;
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop()
{
  /* Reading command from PC to decide what to do next*/
  if (!is_mode_selected)
  {
    while (Serial.available() > 0)
    {
      InBytes = Serial.read();

      if (InBytes == WEIGHT_SCALE_MODE)
      {
        pc_rq_weight_scale = true;
        is_mode_selected = true;
      }
      else if (InBytes == UPLOADING_MODE)
      {
        send_data_i2c(SLAVE_ADDRESS, (uint8_t)UPLOADING_MODE);
        send_data_i2c(SLAVE_ADDRESS, (uint8_t)UPLOADING_MODE);
        pc_rq_upload = true;
        is_mode_selected = true;
      }
      else if (InBytes == ERASE_MODE)
      {
        pc_rq_erase_mem = true;
        is_mode_selected = true;
      }
      else if (InBytes == SYSRESET_MODE)
      {
        pc_rq_sysreset = true;
        is_mode_selected = true;
      }
      else if (InBytes == GO2UPAPP_MODE)
      {
        pc_rq_go2UpApp = true;
        is_mode_selected = true;
      }
      else if (InBytes == UPDMAIN_MODE)
      {
        pc_rq_updmain = true;
        is_mode_selected = true;
      }
    }
  }
  else
  {
    if (pc_rq_weight_scale)
    { // Reading scale
      send_data_i2c(SLAVE_ADDRESS, (uint8_t)WEIGHT_SCALE_MODE); // dont know why but have to send one BYTE fist
      send_data_i2c(SLAVE_ADDRESS, (uint8_t)0);          // then any number follows up for the first byte to be read by the slave
      weight_scale_read(SLAVE_ADDRESS);
    }
    else if (pc_rq_erase_mem)
    {
      send_data_i2c(SLAVE_ADDRESS, (uint8_t)ERASE_MODE); // dont know why but have to send one BYTE fist
      send_data_i2c(SLAVE_ADDRESS, (uint8_t)0);          // then any number follows up for the first byte to be read by the slave
      pc_rq_erase_mem = 0;
      is_mode_selected = 0;
    }
    else if (pc_rq_sysreset)
    {
      send_data_i2c(SLAVE_ADDRESS, (uint8_t)SYSRESET_MODE);
      send_data_i2c(SLAVE_ADDRESS, (uint8_t)0);
      pc_rq_erase_mem = 0;
      is_mode_selected = 0;
    }
    else if (pc_rq_go2UpApp)
    {
      send_data_i2c(SLAVE_ADDRESS, (uint8_t)GO2UPAPP_MODE);
      send_data_i2c(SLAVE_ADDRESS, (uint8_t)0);
      pc_rq_erase_mem = 0;
      is_mode_selected = 0;
    }
    else if (pc_rq_updmain)
    {
      send_data_i2c(SLAVE_ADDRESS, (uint8_t)UPDMAIN_MODE);
      send_data_i2c(SLAVE_ADDRESS, 0);
      pc_rq_updmain = 0;
      is_mode_selected = 0;
    }

    else if (pc_rq_upload)
    { // Uploading
      while (Serial.available() > 0)
      {
        InBytes = Serial.read(); // read byte

        switch (state)
        {
        // read bytecount first
        case READ_BYTECOUNT:
          data[i] = InBytes;
          state = READ_DATAPART;
          // datacount = 1 Bytes, address = 2, recordtype = 1, cs = 1
          line_length = 1 + 2 + 1 + 1 + data[i];
          i++;
          break;

        // read remaining data except cs
        case READ_DATAPART:
          data[i] = InBytes;
          // check if reached checksum
          if (i == (line_length - 2))
          {
            state = READ_CHECKSUM;
          }
          i++;
          break;

        // read and check cs
        case READ_CHECKSUM:
          data[i] = InBytes;
          if (check_cs(data[i], i, data))
          {
            is_pass_cs = true;
          }
          else
            is_pass_cs = false;

          state = READ_END;
          break;

        default:
          break;
        }

        if (state != READ_END)
        { // while reading
        }
        else
        { // when done reading, responding to cs result
          if (!is_pass_cs)
          {
            Serial.print(404); // 404 for fail
          }
          else
          {
            // if cs pass, send data to stm32
            for (int j = 0; j < line_length + 1; j++)
            {
              send_data_i2c(SLAVE_ADDRESS, data[j]);
            }

            int i2c_resp;
            while (1)
            {
              delayMicroseconds(500 / 4);
              i2c_resp = read_data_i2c(SLAVE_ADDRESS);
              if (i2c_resp != 303)
                break; // if receive "not ready" signal 303, loop the request
            }
            Serial.print(i2c_resp);
          }
          // reset  variables
          state = READ_BYTECOUNT;
          i = 0;
        }
      }
    }
  }
}
