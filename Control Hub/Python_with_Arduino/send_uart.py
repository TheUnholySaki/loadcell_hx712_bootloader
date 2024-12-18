import serial
import time

arduino_res = 0
error_count = 0
temp_count = 0
read_success_flag = 0

NUMBER_OF_ATTEMPTS = 100 # number of resend attempt if respond returns fail


def send_data(COM, data_to_send):
    """
    @brief Write in the COM port
    @note  Delay for sometime before doing anything next
    """
    global arduino_res
    COM.write(bytes([data_to_send]))

    ticks = 0
    while (ticks < 35000):  # > 35000 when plugged in, tested
        ticks += 1

    # print("SEND INFO: ", data_to_send, "|| ", bytes([data_to_send]))


def is_sending_success(COM):
    """
    @brief  Check if transmission is succesful by reading MCU responds
    @retval True if responds successful (555), False if not
    """
    if try_to_read(COM, 1):
        return True
    else:
        return False
    
    
def try_to_read(COM, mode):
    """
    @brief Read MCU
    """
    global error_count
    global arduino_res
    global read_success_flag
    try:
    # only when the res is 404 or 555 can the loop be broken
    # delay before read because the action of arduino print out response is not instant
        ticks = 0
        while (ticks < 50000):  # 50000 tested
            ticks += 1
        if read_success_flag != 1:
            if mode == 1:
                arduino_res = int(COM.readline().decode())%1000
            else: 
                arduino_res = int(COM.readline().decode())
        else:
            read_success_flag = 0 # reset flag
            return True

            
    except ValueError:
        ticks = 0
        while (ticks < 3000):
            ticks += 1
        # if read catch NULL, try read again, if fail return fail value
        if try_to_read(COM, mode):
            read_success_flag = 1 #return to main app right away
            return True
        else:
            return False
  
    else:
        if mode == 1:
            if int(arduino_res) == 404 or int(arduino_res) == 405:
                print("Arduino returns cs fail with code: ",
                        arduino_res, "re-send data, tries: ", error_count)

                error_count += 1
                if error_count == NUMBER_OF_ATTEMPTS:  # max 4 attempts to resend the line
                    print("check your code, Im out")
                    exit()
                return False

            # if succeed
            elif int(arduino_res) == 555:
                error_count = 0
                # line has been accepted by arduino and sent to stm32
                # print("Line accepted with response code of: ", int(arduino_res))
                return True
                

            else:
                print("unknown response: ", int(arduino_res))
                error_count += 1
                if error_count == NUMBER_OF_ATTEMPTS:
                    print("check your code, Im out")
                    exit()
                    
        else:
            print("Received weight is: ", float(arduino_res)/100, "g")
            #print("unknown response2: ", int(arduino_res))
    finally:
        pass 
    