#pragma once
#include <winsock2.h>
#include<vector>
#include<set>
#include<fstream>
#include <iostream>

#define PORT 3000
#define SERVER_ADDRESS "127.0.0.1" //update as per the need

// Packet structure
struct Packet {
    char symbol[4];      // 4 bytes
    char buySell;        // 1 byte
    uint32_t quantity;    // 4 bytes
    uint32_t price;       // 4 bytes
    uint32_t sequence;    // 4 bytes
};


class ABXExchangeClient
{
public:
    ABXExchangeClient();

    ~ABXExchangeClient();

    void connectToServer(const std::string& serverAddress, int port);

    void sendStreamRequest();
    
    void receivePackets();

    void requestMissingPackets();

    bool validateReceivedData();
 
    void writeJSONToFile(std::ofstream& outFile);
private:
    SOCKET sock;
    std::vector<Packet> packets;
    std::set<uint32_t> sequences;
};

