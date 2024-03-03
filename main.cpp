#include "headers/FS-IA6B.h"

#include <iostream>

int linearScaling(int val, int min_r1, int max_r1, int min_r2, int max_r2){
    return ((val - min_r1)*(max_r2 - min_r2))/(max_r1 - min_r1) + min_r2;
}

int main()
{
    FS_IA6B controller;
    IBusChannels s_channels{ 0 };

    // min max values of the motor
    const unsigned int MIN_MOTOR_SPEED = 1000;
    const unsigned int MAX_MOTOR_SPEED = 2000;
    
    // min max values of the throttles' controller
    const unsigned int MIN_THROTTLE_CTRL = 1025;
    const unsigned int MAX_THROTTLE_CTRL = 1975;

    unsigned int throttle_val = 0;
    unsigned int thrust = 0;
    while(s_channels.channels[5] < 1900)
    {
        controller.readValues(&s_channels);
        throttle_val = s_channels.channels[2];

        //std::cout << throttle_val << std::endl;
    }

    std::cout << "finished" << std::endl;

    return 0;
}