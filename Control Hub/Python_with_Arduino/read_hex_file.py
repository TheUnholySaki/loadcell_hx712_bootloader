from send_uart import *

COM = serial.Serial('COM3', int(76800/4))
COM.timeout = 0

# define variables
data_startcode = ""
data_bytecount = ""
data_address = ""
data_recordtype = ""
data_datapart = ""
data_checksum = ""
data_EOL = ""

LINE_TO_READ = 9999 # define number of line to read and sent,
                    # set to low when fixing bug

global period
start_time = 1


def send_hex_file(file_path):
    """
    @brief  Read hex file line by line
    If reads line successful, send data line to MCU
    Read MCU responds
    If MCU returns transmission complete, repeat process
    """
    global period
    global start_time
    try:
        with open(file_path, 'rb') as file:

            lines = file.readlines()
            file.seek(0, 0)  # return cursor to start of line

            # read data
            m = 0
            n = 1
            line_count = 0
            for line in file:  # using line loop for check EOF
                period = 0
                start_time = time.time()
                line_count += 1
                # read startcode
                read_and_send_file(line, file, lines, line_count)

                # wait for ACK
                while True:
                    if (is_sending_success(COM) == True):
                        break
                    else:
                        # if return file, resend the line
                        # file.seek(-len(line), 1)
                        print("resend line...")
                        read_and_send_file(line, file, lines, line_count)

                period = time.time() - start_time
                print(" |", round(period*len(lines), 2), "s")
                m += 1
                if m == LINE_TO_READ:
                    break
            print("End of File, Upload complete")
            exit()

    except FileNotFoundError:
        print(f"File not found: {file_path}")
    except IOError:
        print(f"An error occurred while reading the file: {file_path}")


def read_and_send_file(line, file, lines, line_count):
    """
    @brief algorithm for read and send line
    """
    global period
    file.seek(-len(line), 1)  # return cursor to start of line
    print(line_count, "/", len(lines), "|",
          f'{round(line_count*100/len(lines),2):.2f}' + "%", end="")

    read_data = file.read(1).decode()
    if read_data != ":":  # check validation
        print("error")
        return IOError
    else:
        data_startcode = read_data               # read ":"

        data_bytecount = file.read(2).decode()   # read byte count
        data_address = file.read(4).decode()     # read Address
        data_recordtype = file.read(2).decode()  # read Recordtype
        data_datapart = file.read(
            len(line) - 13).decode()             # read data
        data_checksum = file.read(2).decode()    # read checksum
        data_EOL = file.read(2).decode()         # bypass /r/n

    # prepare for checksum
    data_string = data_bytecount + data_address + data_recordtype + data_datapart
    verify_checksum(data_string, data_checksum)

    # prepare to send data
    data_string = data_string + data_checksum
    data = 0
    j = 0

    for _ in range(int(len(data_string)/2)):
        data = int(data_string[j:j+2], base=16)
        j += 2
        send_data(COM, data)
        


def verify_checksum(data_string, pref_cs):
    """
    @brief checksum verification algorithm
    """
    j = 0
    total_checksum = 0

    # half of len because read 2 chars at a time
    for _ in range(int(len(data_string)/2)):
        total_checksum += int(data_string[j:j+2], base=16)
        j += 2

    # %256 for return to 0 if reach 256
    calc_checksum = (256-total_checksum % 256) % 256

    if (calc_checksum == int(pref_cs, base=16)):
        # if success, do nothing, else exit the application
        pass
    else:
        print(data_string, total_checksum, (256-total_checksum %
                                            256) % 256, int(pref_cs, base=16))
        print("checksum not pass, recheck the HEX file")
        exit()
