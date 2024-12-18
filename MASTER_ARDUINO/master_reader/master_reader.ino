#include <Wire.h>

#define SLAVE_ADDRESS 8
#define SW 2
#define READ_BYTECOUNT 1
#define READ_ADDRESS 2
#define READ_RECORDTYPE 3
#define READ_DATAPART 4
#define READ_CHECKSUM 5
#define READ_END 6

#define READ_WEIGHT_SCALE_MODE  0xF1
#define UPLOADING_MODE          0xF2
#define ERASE_MODE              0xF3
#define SYSRESET_MODE           0xF4
#define RAISEFLAG_MODE          0xF5
#define UPDMAIN_MODE            0xF6

bool is_mode_selected = false;
bool pc_rq_weight_scale = false;
bool pc_rq_upload = false;
bool pc_rq_erase_mem = false;
bool pc_rq_sysreset = false;
bool pc_rq_go2UpApp = false;
bool pc_rq_updmain = false;

// VARIABLES FOR HX712
int counter;
double weight;
uint8_t state = 0;


//VARIABLE FOR BOOTLOADER

int cs_flag = 0;
int bytecount;
int checksum;


int data[100];
int InBytes;
int i = 0;
uint8_t line_length = 0;
int i2c_resp = 0;



// =============CODE=============

void setup() {
  Wire.begin();                     // join I2C bus (address optional for master)
  Serial.begin(76800, SERIAL_8N1);  // start serial for output 234000/5
  pinMode(SW, INPUT);
  digitalWrite(SW, HIGH);
  state = READ_BYTECOUNT;
  digitalWrite(LED_BUILTIN, HIGH);
}


void loop() {
  //select mode

  /* Reading command from PC to decide what to do next*/
  if (!is_mode_selected) {
    while (Serial.available() > 0) {
      InBytes = Serial.read();

      if (InBytes == READ_WEIGHT_SCALE_MODE) 
      {
        pc_rq_weight_scale = true;
        is_mode_selected = true;
      } 
      else if (InBytes == UPLOADING_MODE) {
        send_data_i2c(242);
        send_data_i2c(242);
        pc_rq_upload = true;
        is_mode_selected = true;
      } 
      else if (InBytes == ERASE_MODE) {
        pc_rq_erase_mem = true;
        is_mode_selected = true;
      } 
      else if (InBytes == SYSRESET_MODE) {
        pc_rq_sysreset = true;
        is_mode_selected = true;
      } 
      else if (InBytes == RAISEFLAG_MODE) {
        pc_rq_go2UpApp = true;
        is_mode_selected = true;
      }
      else if (InBytes == UPDMAIN_MODE) {
        pc_rq_updmain = true;
        is_mode_selected = true;
      }
    }
  }



  //if command is received
  if (is_mode_selected) {
    if (pc_rq_weight_scale) 
    {  // Reading scale
      weight_scale_read();
    } 
    else if (pc_rq_erase_mem) 
    {
      send_data_i2c(243);  //dont know why but have to send one fist
      send_data_i2c(0);    //then any number follows up for first byte to be read by the slave
      pc_rq_erase_mem = 0;
      is_mode_selected = 0;
    } 
    else if (pc_rq_sysreset) 
    {
      send_data_i2c(244);
      send_data_i2c(0);
      pc_rq_erase_mem = 0;
      is_mode_selected = 0;
    } 
    else if (pc_rq_go2UpApp) 
    {
      send_data_i2c(245);
      send_data_i2c(0);
      pc_rq_erase_mem = 0;
      is_mode_selected = 0;
      digitalWrite(LED_BUILTIN, LOW);
      delay(1);
    }
    else if (pc_rq_updmain) 
    {
      send_data_i2c(246);
      send_data_i2c(0);
      pc_rq_updmain = 0;
      is_mode_selected = 0;
    } 

    else if (pc_rq_upload) 
    {  // Uploading
      while (Serial.available() > 0) 
      {
        InBytes = Serial.read();  // read byte

        switch (state) 
        {
          // read bytecount first
          case READ_BYTECOUNT:
            /* assign the byte to the data buffer
          set flag to remaining data
          determine line_length */
            data[i] = InBytes;
            state = READ_DATAPART;
            //datacount = 1 Bytes, address = 2, recordtype = 1, cs = 1
            line_length = 1 + 2 + 1 + 1 + data[i];
            i++;
            break;
          // read remaining data except cs
          case READ_DATAPART:
            data[i] = InBytes;
            // check if reached checksum
            // if yes, set flag to checksum
            if (i == (line_length - 2)) {
              state = READ_CHECKSUM;
            }
            i++;
            break;
          // read and check cs
          case READ_CHECKSUM:
            data[i] = InBytes;
            // 0: cs pass, 1 cs: fail
            if (check_cs(data[i], i)) {
              cs_flag = 0;
            } else cs_flag = 1;  // 1 = fail
            state = READ_END;
            break;
          default:
            break;
        }

        if (state != READ_END) 
        {  // while reading
        
        } 
        else 
        {                  // when done reading, responding to last bit
          if (cs_flag == 1) 
          {     // if cs return fail
            Serial.print(404);    //404 for fail
          } 
          else 
          {                // cs return completed
            // if cs pass, send data to stm32
            for (int j = 0; j < line_length + 1; j++) 
            {
              send_data_i2c(data[j]);
            }
            //delayMicroseconds(2000/4); // 500/4
            //read return signal and sent to PC
            while (1) 
            {
              delayMicroseconds(500 / 4);
              i2c_resp = read_data_i2c();
              if (i2c_resp == 303)
                ;  //Serial.print(i2c_res);
              else break;
            }
            Serial.print(i2c_resp);  //555 for success
          }
          // reset  variables
          state = READ_BYTECOUNT;
          i = 0;
        }
      }
    }
  }
}
