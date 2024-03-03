#pragma once

// for UART
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include <stdexcept>
#include <iostream>
#include <string.h>
#include <chrono>
#include <math.h>



// IBus data from the FS-IA6B send 2 bytes for the header (0x20 and 0x40) and 2 bytes for the
// verification. (ie checksum) Where the start value is 0xFFFF where we substract all precedent bytes to it.
// The IBus is sending 32 bytes. 28 bytes of data. Each 2 bytes is a channel.

struct IBusChannels{
    uint16_t channels[14]; // useful for the moment is channel 0 to 5
};

class FS_IA6B
{
    private:
        const char* m_device = "/dev/serial0";
        int m_handle;
        std::string m_values;

    public:
        FS_IA6B();
        ~FS_IA6B();

        void readValues(IBusChannels* _ch);

    private:
        void decodeIBusFrame(const char* _frame, size_t _size, IBusChannels* _ch);
        bool checksum(const char* _data, size_t _size);
};