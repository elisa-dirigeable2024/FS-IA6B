#include "../headers/FS-IA6B.h"

FS_IA6B::FS_IA6B(const char* serial_device){

    /* Configuration of the object :
    ================================
    */

    // Get m_device :
    m_device = serial_device;
    //const char* m_device = "/dev/ttyAS5";

    // open the port with the following settings :
    //   - O_RDONLY -> read only 
    //   - O_NOCTTY -> this programm is not the controlling terminal of this port
    //   - O_NDELAY -> don't care about UART's DCD signal
    m_handle = open(m_device, O_RDONLY);// | O_NOCTTY);// | O_NDELAY);
    if(m_handle == -1){
        throw std::runtime_error("Error while opening the UART port");
    }
    if(!isatty(m_handle)){
        throw std::runtime_error("Device specified not Serial");
    }
    std::cout << "[DEBUG] " << "UART port open !" << std::endl;

    /*if(flock(m_handle, LOCK_EX | LOCK_NB) == -1) {
        throw std::runtime_error("Serial port with file descriptor " + 
            std::to_string(m_handle) + " is already locked by another process.");
    }*/

    /* Configuration of the communication :
    =======================================
    */

    // configuration struct of the UART port
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if(tcgetattr(m_handle, &tty) != 0){
        close(m_handle);
        throw std::runtime_error("Error while getting the attribute");
    }

    // Input mode :
    //* tty.c_iflag &= ~INPCK;   // Checking of parity in input
    //tty.c_iflag &= ~IGNPAR;  // Ignore the bytes with parity error
    //tty.c_iflag &= ~PARMRK;  // Mark the bytes with parity error
    //tty.c_iflag &= ~ISTRIP;  // Strip the parity bit of the valide bytes
    //tty.c_iflag &= ~IGNBRK;  // Ignore break conditions
    //tty.c_iflag &= ~BRKINT;   // What to do if break condition is detected
    //tty.c_iflag &= ~IGNCR;    // Ignore carriage return
    //tty.c_iflag &= ~ICRNL;   // Transformation of '\r' caracter
    //tty.c_iflag |= INLCR;   // Transformation of '\n' caracter
    tty.c_iflag &= ~IXOFF;   //* Start/Stop control on input
    tty.c_iflag &= ~IXON;    //* Start/Stop control on output
    tty.c_iflag &= ~IXANY;   //* Restart output conditions
    //tty.c_iflag |= IMAXBEL;  // BEL caracter if input buffer is full

    // Control Modes
    // * tty.c_cflag |= CLOCAL;   // Indicate local terminal
    //tty.c_cflag &= ~HUPCL;   // Modem disconnect condition behavior
    // * tty.c_cflag |= CREAD;    // Input reaction
    tty.c_cflag &= ~CSTOPB;  // Clear stop field -> only one stop bit is used (and not two)
    tty.c_cflag &= ~PARENB;  // Clear parity bit -> disabling parity
    //tty.c_cflag &= ~PARODD;  // Odd or Even parity used
    tty.c_cflag &= ~CSIZE;   // Clear all size bit
    tty.c_cflag |= CS8;      // Set the new size bit wanted
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)

    // Local Modes
    tty.c_lflag &= ~ICANON;  // Clear canonical bit -> Disabling special treating for some characters
    tty.c_lflag &= ~ECHO;    //* Disable echo
    tty.c_lflag &= ~ECHOE;   //* Disable erasure
    //tty.c_lflag &= ~ECHOPRT; // Display of the ERASE caracter ('\')
    //tty.c_lflag &= ~ECHOK;   // Special display for KILL caracter
    //tty.c_lflag &= ~ECHOKE;  // Other special display for KILL caracter
    //tty.c_lflag &= ~ECHONL;  // Disable new-line echo
    //tty.c_lflag &= ~ECHOCTL; // Display behavior of CTRL ('^') caracter
    tty.c_lflag &= ~ISIG;    // Disable interpretation of INTR, QUIT and SUSP
    //tty.c_lflag &= ~IEXTEN;  // Enabled or disabled of some special caracter
    //tty.c_lflag &= ~NOFLSH;  // Flush behavior after some caracter
    //tty.c_lflag |= TOSTOP;   // Generation of signal if an other process try to access the terminal
    //tty.c_lflag |= FLUSHO;   // Behavior with DISCARD caracter
    //tty.c_lflag &= ~PENDIN;  // Reprint behavior

    // VMIN and VTIME
    //tty.c_cc[VTIME] = 10;  // In deca second, so 10ds = 1s
    //tty.c_cc[VMIN] = 0;    // Minimal data befor return, 0 means return as soon as their is data in the buffer

    // Set IN speed
    if(cfsetispeed(&tty, B115200) < 0){
        close(m_handle);
        throw std::runtime_error("Error while affecting the input speed");
    } 

    // Saving the configuration
    if(tcsetattr(m_handle, TCSANOW, &tty) != 0){
        close(m_handle);
        throw std::runtime_error("Failed to set attribute to the uart handle");
    }
    std::cout << "[DEBUG] config is done" << std::endl;
}

FS_IA6B::~FS_IA6B(){
    close(m_handle);
}

void FS_IA6B::readValues(IBusChannels* _ch){

    // this variable will store all buffers into a single variable
    // until the next header
    // the data are sent 8 bits by 8 bits. But we use a bigger in case.
    char buffer[33];

    // each data are sent every 7 ms. We have to track the time and sleep to wait 7 ms.
    std::chrono::steady_clock::time_point begin;
    std::chrono::steady_clock::time_point end;

    while(_ch->channels[5] < 1900){
        begin = std::chrono::steady_clock::now();

        // the function returns the number of bytes read
        ssize_t bytes_read = read(m_handle, &buffer, sizeof(buffer) - 1);

        if(bytes_read > 0){

            // EOF at the end of the buffer
            buffer[bytes_read] = '\0';
            
            // Start reading until the EOF symbol is found :
            char* value = &buffer[0];
            while(*value != '\0'){

                // If we find the start of a new_message
                if(*value == 0x20 && *(value + 1) == 0x40){
                    // We decode the message :
                    decodeIBusFrame(m_values.c_str(), m_values.size(), _ch);
                    // then we clear it and start reading again :
                    m_values.clear();
                    m_values.push_back(*value);
                }

                // If we don't find the start of a new message :
                else{
                    // we add the new value to the message :
                    m_values.push_back(*value);
                }
                value++;
            }

        }
        if(bytes_read < 0){
                // error in reading data
                std::cout << "Error :: readed " << bytes_read << "bytes of data" << std::endl;
        }
        end = std::chrono::steady_clock::now();
        int64_t delta = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count(); // ms
        if(delta*1000 < 7000){
            usleep(7000 - delta * 1000);
        }
    }
}

void FS_IA6B::decodeIBusFrame(const char* _frame, size_t _size, IBusChannels* _ch){
    int ibus_count = 0; // counter for each channels

// [DEBUG]
/*
    std::cout << "New message (" << _size << ") : ";

    // Show headers :
    std::cout << "(" << std::hex << (int)_frame[0];
    std::cout << "|" << std::hex << (int)_frame[1];
    std::cout << ") ";

    // Show values :
    for (size_t ii = 2; ii<_size; ii+=2){
        std::cout << std::hex << (int)_frame[ii];
        std::cout << std::hex << (int)_frame[ii+1];
        std::cout << "  ";
    }

    // Show cheksum :
    std::cout << "[" << std::hex << (int)_frame[_size-2];
    std::cout << "|" << std::hex << (int)_frame[_size-1];
    std::cout << "] "<< std::endl;

    // Check cheksum :
    //bool is_valid = compute_checksum(msg.c_str(),msg_size);
    //std::cout << "-> " << is_valid << std::endl;
*/
// [DEBUG]

    if(_frame[0] == 0x20 && _frame[1] == 0x40 && checksum(_frame, _size)) // check the header
    {

        // start after the header and stop before the checksum
        for(size_t i = 2; i < _size - 2; i += 2){
            uint16_t ch_val = 0;
            int first = static_cast<int>(_frame[i]);
            int second = static_cast<int>(_frame[i + 1]);

            ch_val |= first;
            ch_val |= (second << 8);
            _ch->channels[ibus_count] = ch_val;
            ibus_count++;
        }
    }
    else {
	//std::cout << "Cheksum NOK :(" << std::endl;
    }
}

/*
    The checksum is a special value at the end of the buffer to check if the data aren't corrupted.
    The algorithm for the FS-IA6B is to take the value of 0xFFFF and substract each channel's data 
    except the checksum itself.
*/
bool FS_IA6B::checksum(const char* _data, size_t _size){
    uint16_t checksum = 0xFFFF;
    for(size_t i = 0; i < _size - 2; i++){ // we exclude the checksum
        checksum -= static_cast<uint8_t>(_data[i]);
    }

    uint16_t real_checksum = 0;
    int first = static_cast<int>(_data[_size - 2]);
    int second = static_cast<int>(_data[_size - 1]);

    real_checksum |= first;
    real_checksum |= (second << 8);

    if(real_checksum == checksum){
        return true;
    }
    return false;
}
