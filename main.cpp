#include "headers/FS-IA6B.h"
#include <SimpleAmqpClient/SimpleAmqpClient.h>

#include <thread>
#include <iostream>
#include <unistd.h>
#include <sys/file.h>

#include <fcntl.h>
#include <termios.h>
#include <stdexcept>
#include <string.h>
#include <chrono>
#include <math.h>



int main()
{
    /* Configuration of the controller object :
    ===========================================
    */

    // Telecommande object :
    FS_IA6B controller = FS_IA6B("/dev/ttyAS5");

    // IBus Channel object :
    IBusChannels s_channels{ 0 };


    /* Configuration of the broker connection object :
    ==================================================
    */

   // Connexion parameters :
   std::string host = "localhost";
   int port = 5672;
   std::string user_name = "guest";
   std::string user_pwd  = "guest";

   // Connexion :
   AmqpClient::Channel::ptr_t channel = AmqpClient::Channel::Create(host, port, user_name, user_pwd);

   // Message to send :
   std::string msg_speed;
   std::string msg_angle;
   


    // Reading from the UART port :
    // ============================
    std::thread reading_thread([&]() {
        std::cout << "Start reading ...\n";
        controller.readValues(&s_channels);
        std::cout << "Done reading\n";
    });


   std::cout << "Starting while loop ...\n";
    while(s_channels.channels[5] < 1900)
    {
        //c0 = s_channels.channels[0];
	    //c1 = s_channels.channels[1];
	    //c2 = s_channels.channels[2];
	    //c3 = s_channels.channels[3];
	    //c4 = s_channels.channels[4];
	    //c5 = s_channels.channels[5];
	    //c6 = s_channels.channels[6];

        // get values :
        msg_speed = std::to_string(s_channels.channels[2]);
        msg_angle = std::to_string(s_channels.channels[1]);

        // send the message :
        channel->BasicPublish("amq.topic","speed.nnMap", AmqpClient::BasicMessage::Create(msg_speed));
        channel->BasicPublish("amq.topic","angle.nnMap", AmqpClient::BasicMessage::Create(msg_angle));

        // Print for debug purposes :
	    std::cout << "c_0 = " << s_channels.channels[0] << "\t";
	    std::cout << "c_1 = " << s_channels.channels[1] << "\t";
	    std::cout << "c_2 = " << s_channels.channels[2] << "\t";
	    std::cout << "c_3 = " << s_channels.channels[3] << "\t";
	    std::cout << "c_4 = " << s_channels.channels[4] << "\t";
	    std::cout << "c_5 = " << s_channels.channels[5] << "\t";
	    std::cout << "c_6 = " << s_channels.channels[6] << "\t";
	    std::cout << "c_7 = " << s_channels.channels[7] << "\t";
	    std::cout << "c_8 = " << s_channels.channels[8] << "\t";
	    std::cout << "c_9 = " << s_channels.channels[9] << std::endl;
    	usleep(10000); // wait 10ms
    }
    reading_thread.join(); // wait the thread
    std::cout << "finished" << std::endl;

    return 0;
}
