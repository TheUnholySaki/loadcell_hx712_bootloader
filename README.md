# loadcell_hx712_bootloader

***This is an internship project***

An loadcell application using HX712 ADC module
Additionally, the main part about this project is bootloader
implemented in the STM32F103C8T6. The hex file is generated 
and sent from a Python application running in Windows. Also,
to put more complexity, PC will communicate with STM32 through
a master I2C Arduino. The flow is below:

{PC} <====UART====> {Arduino} <====I2C====> {STM32}

You can find the source code as follows:
PC     :    1. Control Hub
Arduino:    1. Arduino              #
STM32  :    1. BootLoader-main      # Bootloader image
            2. Application          # Main application image
            3. ApplicationUpdate    # Update application imaged 
                                    # (Optional)

![alt text](<Memory map.jpg>)

Also, please find the document I included in for more infomation.


