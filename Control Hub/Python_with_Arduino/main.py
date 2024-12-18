from send_uart import *
from read_hex_file import *
import keyboard

UPLOADING_MODE = 0xF2
WEIGHT_SCALE_MODE = 0xF1
ERASE_MODE = 0xF3
SYSRESET_MODE = 0xF4
RAISEFLAG_MODE = 0xF5
UPDMAIN_MODE = 0xF6

def select_file():
    """
    @brief Select HEX file to upload if this option is selection
    """
    print("Wait for erasing FLASH (~3s)")
    COM.write(bytes([ERASE_MODE]))     # erase for ready to upload
    time.sleep(3)                      # wait for complete erase
    
    COM.write(bytes([UPLOADING_MODE])) # signalling uploading mode
    file = input(
                "\nInput number: \
                \nFile 1: Write on Update image, file 40Hz \
                \nFile 2: Write on main image, file 10Hz\
                \nFile 3: Write on Update image, file testing\
                \nFile 4: input dir\
                \nChoosing mode: ")
    match file:
        case "1":
            send_hex_file('Python_with_Arduino\ApplicationUpdate.hex')# type: ignore

        case "2":
            send_hex_file('Python_with_Arduino\Application.hex')  # type: ignore

        case "3":
            send_hex_file('Python_with_Arduino\BlinkyUpdate.hex')# type: ignore
            
        case "4":
            address = input("Input directory: ")
            send_hex_file(address)

        case _:
            print("Unknown command, try again")
            


def select_command():
    """
    @brief selecting control cammond if this option is selected
    """
    while(1):
        command_mode = input(
                    "\nMode 1: weight_scale \
                    \nMode 2: Erase main memory\
                    \nMode 3: STM32 reset\
                    \nMode 4: Raise goto update app flag\
                    \nMode 5: Raise update main app \
                    \nReturning: write \"return\" \
                    \nChoosing mode: ")
        match command_mode:
            case "1": #weight_scale_mode
                COM.write(bytes([WEIGHT_SCALE_MODE]))
                while(1):
                    try_to_read(COM, 2)
                    try:
                        if keyboard.is_pressed('a'):
                            break
                    except:
                        print("except raised")
                        
            case "2":
                COM.write(bytes([ERASE_MODE]))
                time.sleep(1)
                
            case "3":
                COM.write(bytes([SYSRESET_MODE]))
                time.sleep(1)
                
            case "4":
                COM.write(bytes([RAISEFLAG_MODE]))
            
            case "5":
                COM.write(bytes([UPDMAIN_MODE]))
            
            case "return":
                break
            
            case _:
                print("Unknown command, try again")
            

def main():
    #Main loop
    print("wait for uart to start (about 3s)")
    time.sleep(3)
    while True:
        """
        After running application, the UART com will be initialzed
        After the init, user can choose to 
        upload data or using weight scale application, etc.
        """
        
        mode = input("\nSelect mode: \
                    \nMode 1: uploading \
                    \nMode 2: other specific task \
                    \nExiting: write\"exit\" \
                    \nChoosing mode: ")
        match mode:
            case "1":
                select_file()

            case "2":
                select_command()
                
            case "exit":
                exit()
            
            case _:
                print("Unknown command, try again")
        
        # reset arduino before closing        
        COM.close()
        COM.open()
            
    
if __name__ == "__main__":
    main()
