#include "../headers/FS-IA6B.h"

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

    tty.c_cflag &= ~(PARENB | CSTOPB | CS8); // No parity, 1 stop bit, 8 data bits

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
    
    // the value is read 8 by 8 but we create a larger buffer in special case
    char buffer[33]; 
    // this variable will tell the program when to start to save the values
    // (ie when the first header appear)
    bool finish = false;
    while(!finish){
        // the function returns the number of bytes read
        ssize_t bytes_read = read(m_handle, &buffer, sizeof(buffer) - 1);

        if(bytes_read > 0){
            buffer[bytes_read] = '\0';

            char* value = &buffer[0];
            while(*value != '\0'){
                if(*value != 0x20){
                    m_values.push_back(*value);
                    //std::cout << std::hex << static_cast<int>(*value) << std::endl;
                }
                else{
                    for(int i = 0; i < m_values.size(); i++){
                        std::cout << std::hex << static_cast<int>(m_values[i]) << " ";
                    }
                    std::cout << std::endl;
                    decodeIBusFrame(m_values.c_str(), m_values.size(), _ch);
                    m_values.clear();
                    m_values.push_back(*value);

                    finish = true;
                }
                value++;
            }
        }
        if(bytes_read < 0){
                // error in reading data
        }  
    }
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