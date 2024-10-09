// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "ABXExchangeClient.h"
using namespace std;


int main()
{
    try
    {
        ABXExchangeClient client;
        std::cout << "Connecting to the Server" << std::endl;
        client.connectToServer(SERVER_ADDRESS, PORT);

        std::cout << "Sending stream request for all Symbols to the server" << std::endl;
        client.sendStreamRequest();

        std::cout << "Receiving packets from the server" << std::endl;
        client.receivePackets();

        std::cout << "Requesting for missing packets" << std::endl;
        client.requestMissingPackets();

        std::cout << "Receiving missing packets from the server" << std::endl;
        client.receivePackets();

 
        if (client.validateReceivedData())
        {
            std::cout << "Writing received packet data to JSON File" << std::endl;
            std::ofstream outFile;
            outFile.open("output.json", ios::out);

            client.writeJSONToFile(outFile);
            std::cout << "Data written to JSON file" << std::endl;
        }
        else
            throw "Invalid packets data received";

    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
