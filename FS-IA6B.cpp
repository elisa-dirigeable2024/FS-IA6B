#include "FS-IA6B.h"

FS_IA6B::FS_IA6B(){
    // open the port in read mode only
    m_handle = open(m_device, O_RDONLY);
    if(m_handle == -1){
        throw std::runtime_error("Error while opening the UART port");
    }

    // configuration of the port serie
    struct termios tty;
    if(tcgetattr(m_handle, &tty) < 0){
        close(m_handle);
        throw std::runtime_error("Error while getting the attribute");
    }
    if(cfsetispeed(&tty, B115200) < 0){
        close(m_handle);
        throw std::runtime_error("Error while affecting the input speed");
    }

    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE); // No parity, 1 stop bit, 8 data bits
    tty.c_cflag |= CS8 | CREAD | CLOCAL;       // 8 data bits, enable reading, ignore modem control

    tty.c_cflag &= ~CRTSCTS; // Disable hardware flow control

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE); // Non-canonical mode, disable echo and erase

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Disable software flow control

    tty.c_cc[VMIN] = 1;  // Wait for at least 1 character
    tty.c_cc[VTIME] = 0; // Timeout: 0 (immediate return)

    if(tcsetattr(m_handle, TCSANOW, &tty) < 0){
        close(m_handle);
        throw std::runtime_error("Failed to set attribute to the uart handle");
    }
}

FS_IA6B::~FS_IA6B(){
    close(m_handle);
}

void FS_IA6B::readValues(IBusChannels* _ch){
    // this variable will store all buffers into a single variable
    // until the next header
    std::string values;
    // this variable will tell the program when to start to save the values
    // (ie when the first header appear)
    bool first_buf = false;

    // TODO: find a button
    while(_ch->channels[5] < 1990){
        // the value is read 8 by 8 but we create a larger buffer in special case
        char buffer[256]; 
        // the function returns the number of bytes read
        ssize_t bytes_read = read(m_handle, &buffer, sizeof(buffer));
    
        if(bytes_read > 0){
            // headers are 0x20 and 0x40
            // all the data have a size of 32 usually otherwise it's maybe an error
            if((buffer[0] == 0x20 && buffer[1] == 0x40) || values.size() > 32){
                if(!first_buf) // if we find the first header
                    first_buf = true;
                else if(values.size() == 32){
                    const char* all_buf = values.c_str();
                    decodeIBusFrame(all_buf, values.size(), _ch);
                    
                    values.clear();
                }
                else{ // no good data
                    values.clear();
                }
            }
            if(first_buf){
                for(ssize_t j = 0; j < bytes_read; j++){
                    if(buffer[j] != '\0') // we skip the terminal character
                        values.push_back(buffer[j]);
                }
            }
        }
        if(bytes_read < 0){
            // error in reading data
        }    
    }

    std::cout << "finished" << std::endl;
}

void FS_IA6B::decodeIBusFrame(const char* _frame, size_t _size, IBusChannels* _ch){
    int ibus_count = 0; // counter for each channels
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