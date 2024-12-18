#include "main.h"
#include "mem_jump_function.h"
#include "vartype_convert.h"



/*============================CONSTANTS====================================*/
#define FLASH_MAIN_APP_ADDR   0x8008000
#define FLASH_UPDATE_APP_ADDR 0x08010000
#define FLASH_FLAGS_ADDR      0x08010000
#define JUMP2UPD_FLAG_ADDR    0x0801F000
#define UPDMAIN_FLAG_ADDR     0x0801F004
#define CURRENT_IMAGE         0x0801F008
#define TXDATA_SIZE           10

#define READ_BYTECOUNT        1
#define READ_ADR              2
#define READ_TYPE             3
#define READ_DATAPART         4
#define READ_CS               5
#define READ_END              6
#define PREF_KEY              0x12345600

#define FAIL_CODE             404
#define SUCCESS_CODE          555
#define UPLOADING_CODE        303
/*============================VARIABLES====================================*/
uint8_t receive_data[1];
uint8_t is_mem_erased = 0;
uint8_t TxData[TXDATA_SIZE] = {};
uint32_t result = 0;
uint8_t read_data[4];
uint8_t start_flag = 0;

uint32_t key = 0x00;
uint8_t correct_key = 0;

uint8_t i = 0;
uint8_t state = READ_BYTECOUNT;
uint8_t data[100];
uint8_t line_length;

uint8_t data_type = 10;
uint8_t is_uploading = 0;
uint32_t main_address;
uint32_t address;

uint8_t itr_rq_upload = 0;
uint8_t itr_rq_erase = 0;

/*============================FUNCTION VARIABLES====================================*/

typedef void (*pfunction)(void);
I2C_HandleTypeDef hi2c1;

/*============================DECLARE FUNCTIONS====================================*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);

void update_main_app(void);
uint8_t check_cs(uint8_t cs, uint8_t len);
void process_rx_data(void);
void erase_memory(uint32_t adr, uint32_t size);

/*============================FUNCTIONS====================================*/

/**
 * @brief replace the main image with the update image
 * When bootloader see that user want to replace
 * @note 1. clear the indicator to prevent looping
 * @note 2. delete the main image
 * @note 3. read update image and write main image word by word
 */
void update_main_app(void)
{
  uint32_t read_adr_ptr = FLASH_UPDATE_APP_ADDR;
  uint32_t write_adr_ptr;
  uint32_t word = 0;
  uint8_t byte[4];

  __disable_irq();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);

  // 1. clear flag
  HAL_FLASH_Unlock();
  FLASH_PageErase(UPDMAIN_FLAG_ADDR + 4);
  CLEAR_BIT(FLASH->CR, (FLASH_CR_PER));
  HAL_FLASH_Lock();

  // 2. erase main image
  write_adr_ptr = FLASH_MAIN_APP_ADDR + 4; // +4 to prevent erase bootloader image
  while (write_adr_ptr < FLASH_UPDATE_APP_ADDR)
  {
    HAL_FLASH_Unlock();
    FLASH_PageErase(write_adr_ptr);
    CLEAR_BIT(FLASH->CR, (FLASH_CR_PER));
    write_adr_ptr += 0x400; // 1 PAGE = 1024 byte = 4x16^2 =0x400
    HAL_FLASH_Lock();
  }

  // 3. read update image and write main image word by word
  write_adr_ptr = FLASH_MAIN_APP_ADDR; // reset the adr ptr
  while (write_adr_ptr < FLASH_UPDATE_APP_ADDR)
  {
    for (int i = 0; i < 4; i++) // read 4 bytes to combine into 1 word
    {
      byte[i] = *(uint8_t *)read_adr_ptr;
      read_adr_ptr++;
    }
    word = (byte[3] << 24) | (byte[2] << 16) | (byte[1] << 8) | byte[0];

    HAL_FLASH_Unlock();
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, write_adr_ptr, word);
    CLEAR_BIT(FLASH->CR, (FLASH_CR_PG));
    HAL_FLASH_Lock();

    write_adr_ptr += 0x04;
    word = 0;
  }

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
  __enable_irq();
}

/*------------------------------------------------------------*/

/**
 * @brief read and upload the data to flash
 * @note The data will be devided into 5 parts, using switch case:
 * @note data length, data address, data type, flashing data, checksum
 * because each interrupt only sent 1 byte,
 * counter "i" as well as all used variables are defined globally
 * and will be reset at the end of successful read if needed
 * @note Write UPLOADING_CODE to buffer TxData,
 * which will be sent if requested uploading status by master:
 * @note 303: currently uploading
        404: Uploading fail - may checksum fail or any fail
        555: Read Uploading successful
 */
void process_rx_data(void)
{
  // send pending code
  int_to_str(UPLOADING_CODE, TxData);

  if (start_flag < 2) // dont know why but first 2 bytes of each data line is blank
    start_flag++;
  else
  {
    switch (state)
    {
    /* First part: read data length*/
    case READ_BYTECOUNT:
      data[i] = receive_data[0];
      state = READ_ADR;
      // 1 for bytecount, 2 for adr, 1 for data type and 1 for cs
      line_length = 1 + 2 + 1 + 1 + data[i];

      i++;
      break;

    /* Second part: read address if data is intended to be uploaded*/
    case READ_ADR:
      data[i] = receive_data[0];
      address += data[i];
      address = address << 8;
      if (i == 2)
        state = READ_TYPE;

      i++;
      break;

    /* Third part: datatype*/
    case READ_TYPE:
      data[i] = receive_data[0];
      // data received is flashing data
      if (data[i] == 0)
        data_type = 0;

      // data received is the upper address
      else if (data[i] == 4)
        data_type = 4;

      // data received is end of file
      else if (data[i] == 1)
      {
        int_to_str(SUCCESS_CODE, TxData);
        state = READ_END;
        break;
      }
      // data received is verification key
      else if (data[i] == 6)
      {
        data_type = 6;
      }
      // unknown data type
      else
      {
        data_type = 10;
      }
      state = READ_DATAPART;

      i++;
      break;

    /* Fourth part: data information according to data type*/
    case READ_DATAPART:
      data[i] = receive_data[0];
      // if data received is the upper address
      if (data_type == 4)
      {
        main_address += data[i];
        main_address = main_address << 8;
      }
      // if data received is the verification key
      if (data_type == 6)
      {
        key += data[i];
        key = key << 8;
      }

      if (i == (line_length - 2))
      {
        state = READ_CS;
      }

      i++;
      break;

    /* Fifth part: data line  checksum*/
    case READ_CS:
      data[i] = receive_data[0];

      // if pass checksum
      if (check_cs(data[i], i))
      {
        // if dataline is verification
        if (data_type == 6)
        {
          if (key == PREF_KEY)
          {
            correct_key = 1;
            int_to_str(SUCCESS_CODE, TxData);
          }
          else
            int_to_str(405, TxData); // 405 for key not matched
        }
        // if dataline is upload data
        else if (data_type == 0 && correct_key == 1)
        {
          address = main_address << 8 | (address >> 8);
          uint8_t counter = 0;
          for (int m = 0; m < data[0]; m++)
          {
            read_data[counter] = data[m + 4];
            counter++;
            if (counter == 4)
            {
              result = (read_data[3] << 24) | (read_data[2] << 16) | (read_data[1] << 8) | read_data[0];
              HAL_FLASH_Unlock();
              HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, result);
              CLEAR_BIT(FLASH->CR, (FLASH_CR_PER));
              HAL_FLASH_Lock();
              address += 0x04;

              /* RESET VARIABLE*/
              result = 0;
              counter = 0;
            }
          }
          int_to_str(SUCCESS_CODE, TxData);
        }
        // if dataline is 04 or 01
        else if (data_type != 10)
        {
          int_to_str(SUCCESS_CODE, TxData);
        }
      }
      else // if not pass checksum
      {
        int_to_str(FAIL_CODE, TxData);
      }

      //  reset variables for each dataline read
      data_type = 10;
      state = READ_BYTECOUNT;
      i = 0;
      address = 0;
      start_flag = 1;
      break;

    case READ_END:
      //  reset variables for each dataline read
      data_type = 10;
      state = READ_BYTECOUNT;
      i = 0;
      address = 0;

      // reset other variables
      is_uploading = 0;
      is_mem_erased = 0;
      main_address = 0;
      result = 0;
      start_flag = 0;
      correct_key = 0;

      // End of upload
      int_to_str(SUCCESS_CODE, TxData);
      break;
    }
  }
}

/*------------------------------------------------------------*/
/**
 * @brief Checksum algorithm
 * @param cs reference checksum
 * @param len data length
 */
uint8_t check_cs(uint8_t cs, uint8_t len)
{
  // combine all data
  int total_cs = 0;
  for (uint8_t i = 0; i < len; i++)
  {
    total_cs += data[i];
  }

  uint8_t calc_cs = (256 - total_cs % 256) % 256;
  // compare with received checksums
  if (calc_cs == cs)
    return 1;
  else
    return 0;
}

/*------------------------------------------------------------*/

/**
 * @brief Erase memory section (32kB)
 * @param adr start of memory section
 * @param size how much data to be erased
 */
void erase_memory(uint32_t adr, uint32_t size)
{
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
  uint32_t start_adr = adr;
  while (start_adr < adr + size)
  {
    HAL_FLASH_Unlock();
    FLASH_PageErase(start_adr);
    CLEAR_BIT(FLASH->CR, (FLASH_CR_PER));
    HAL_FLASH_Lock();

    start_adr += 0x400; // 1 PAGE
  }

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
}

/* -------------------i2c interrupt -------------------------------------------*/
/* Enable I2C whenever Master calls, not only when startup
If not enable, I2C is closed permanently after I2C close*/
void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
  HAL_I2C_EnableListen_IT(hi2c);
}

/**
 * @brief Send/Receive data
 * @note 243: erase command
 * @note 242: upload command
 */
void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
  /*Master requests data*/
  if (TransferDirection == I2C_DIRECTION_RECEIVE)
  {
    HAL_I2C_Slave_Seq_Transmit_IT(&hi2c1, TxData, TXDATA_SIZE, I2C_FIRST_FRAME);
  }
  /*Master sends data*/
  else if (TransferDirection == I2C_DIRECTION_TRANSMIT)
  {

    HAL_I2C_Slave_Seq_Receive_IT(&hi2c1, receive_data, 1, I2C_FIRST_AND_LAST_FRAME);
    if (receive_data[0] == 243 && is_mem_erased != 1)
    {
      itr_rq_erase = 1;
    }
    else if (receive_data[0] == 242 || is_uploading == 1)
    {
      itr_rq_upload = 1;
    }
  }
}

/*============================MAIN====================================*/

int main(void)
{
  // Initialize
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();

  HAL_I2C_EnableListen_IT(&hi2c1); // enable I2C istenning
  HAL_I2C_EV_IRQHandler(&hi2c1);   // enable I2C interrupt

  // main loop
  while (1)
  {
    //======== updating mode
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET)
    {
      while (1)
      {
        // host will request delete memory first
        if (itr_rq_erase)
        {
          if (is_mem_erased == 0)
          {
            is_mem_erased = 1;
            erase_memory(FLASH_UPDATE_APP_ADDR, 0x8000);
            erase_memory(FLASH_MAIN_APP_ADDR, 0x8000);
            erase_memory(FLASH_FLAGS_ADDR, 0x8000);
          }

          itr_rq_erase = 0;
        }
        // host then request uploading data
        if (itr_rq_upload)
        {
          if (is_mem_erased == 1)
          {
            is_uploading = 1;
            process_rx_data();
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_7);
            itr_rq_upload = 0; // prevent write one line multiple times

            if (is_uploading == 0)
              break; // done uploading
          }
        }
      }
    }
    // ======= Go to the applications
    // read the flag
    uint8_t read_indicator[2];
    read_indicator[0] = *(uint8_t *)UPDMAIN_FLAG_ADDR;
    read_indicator[1] = *(uint8_t *)JUMP2UPD_FLAG_ADDR;

    if (read_indicator[0] == 0x01)
    {
      update_main_app();
    }
    // if there is a flag, go to update application
    // otherwise go to main application
    if (read_indicator[1] == 0x01)
    {
      go_to_update_app(FLASH_UPDATE_APP_ADDR, JUMP2UPD_FLAG_ADDR);
    }
    // if no update app image to go, jump to main image
    go_to_main_app(FLASH_MAIN_APP_ADDR);
  }
}

/*============================init====================================*/

/* --------------------------------------------------------------------------------*/

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 16;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_ENABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */ // Pin để báo hiệu
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
