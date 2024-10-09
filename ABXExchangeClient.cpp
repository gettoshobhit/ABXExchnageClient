// ABXExchangeClient.cpp
//

#include "ABXExchangeClient.h"
#include <ws2tcpip.h>
#include<boost/json.hpp>
#include<string>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
namespace json = boost::json;

#define PACKET_SIZE 17 // Total size of one packet (4+1+4+4+4)

ABXExchangeClient::ABXExchangeClient() : sock(INVALID_SOCKET) {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
}

ABXExchangeClient::~ABXExchangeClient() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }
    WSACleanup();
}

void ABXExchangeClient::connectToServer(const std::string& serverAddress, int port) {
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        throw std::runtime_error("Socket creation failed");
    }

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
     // Resolve hostname to IP address
    if (inet_pton(AF_INET, serverAddress.c_str(), &server.sin_addr) <= 0) {
        struct addrinfo hints = {}, * res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        // Resolve the server address and port
        if (getaddrinfo(serverAddress.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) {
            std::cerr << "Could not resolve hostname: " << serverAddress << std::endl;
            closesocket(sock);
            WSACleanup();
            return;
        }

        // Copy the first resolved address to server
        sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(res->ai_addr);
        server.sin_addr = addr->sin_addr;

        freeaddrinfo(res);
    }

    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        closesocket(sock);
        throw std::runtime_error("Connection failed");
    }
}

void ABXExchangeClient::sendStreamRequest()
{
    uint8_t request[2] = { 1, 0 }; // Call type 1, resendSeq not used
    if (send(sock, reinterpret_cast<const char*>(request), sizeof(request), 0) == SOCKET_ERROR)
    {
        throw std::runtime_error("sendStreamRequest failed");
    }
}

void ABXExchangeClient::receivePackets() {
    char buffer[PACKET_SIZE];
    while (true) {
        size_t bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            break;
        }
        Packet packet;
        for (int i = 0; i <= 3; i++)
            packet.symbol[i] = buffer[i];
 
        packet.buySell = buffer[4];
        char quantity[4];
        for (int i = 5; i <= 8; i++)
        {
            quantity[i - 5] = buffer[i];
        }

        packet.quantity = ntohl(*(uint32_t*)quantity);

        char price[4];
        for (int i = 9; i <= 12; i++)
        {
            price[i - 9] = buffer[i];
        }

        packet.price = ntohl(*(uint32_t*)price);

        char sequence[4];

        for (int i = 13; i <= 16; i++)
        {
            sequence[i - 13] = buffer[i];
        }

        packet.sequence = ntohl(*(uint32_t*)sequence);
        sequences.insert(packet.sequence);
        packets.push_back(packet);

        std::cout << "Received Packet: "
            << "Symbol: " << std::string(packet.symbol, 4)
            << ", Buy/Sell: " << packet.buySell
            << ", Quantity: " << packet.quantity
            << ", Price: " << packet.price
            << ", Sequence: " << packet.sequence << std::endl;
     }
}

void ABXExchangeClient::requestMissingPackets() {
    connectToServer(SERVER_ADDRESS, PORT);
    for (uint32_t i = 1; i < *sequences.rbegin(); ++i) {
        if (sequences.find(i) == sequences.end())
        {
            uint8_t request[2] = { 2, static_cast<uint8_t>(i) }; // Call type 2
            if (send(sock, reinterpret_cast<const char*>(request), sizeof(request), 0) == SOCKET_ERROR)
            {
                throw std::runtime_error("requestMissingPackets failed");
            }
        }
    }
    // shutdown the send half of the connection since no more data will be sent
    int32_t iResult = shutdown(sock, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
    }
}

bool ABXExchangeClient::validateReceivedData()
{
    for (uint32_t i = 1; i < *sequences.rbegin(); ++i) {
        if (sequences.find(i) == sequences.end())
        {
            return false;
        }
    }

    for (uint32_t i = 0; i < packets.size(); i++)
    {
        if (std::string(packets[i].symbol, sizeof(packets[i].symbol)).empty()
            || !(packets[i].buySell == 'B' || packets[i].buySell == 'S')
            || packets[i].quantity <= 0
            || packets[i].price <= 0
            || packets[i].sequence <= 0)
            return false;
    }
    return true;
}

void ABXExchangeClient::writeJSONToFile(std::ofstream& outFile)
{
    std::sort(packets.begin(), packets.end(), [](Packet& packet1, Packet& packet2) { return packet1.sequence < packet2.sequence; });

    if (outFile.is_open())
    {
        json::array jsonArray;
        for (const auto& packet : packets) {
            json::object obj;
            obj["Symbol"] = std::string(packet.symbol, sizeof(packet.symbol));
            obj["buySell"] = std::string(1, packet.buySell);
            obj["quantity"] = packet.quantity;
            obj["Price"] = packet.price;
            obj["Sequence"] = packet.sequence;
            jsonArray.push_back(obj);
        }
        json::value jv = jsonArray;
        std::string jsonString = json::serialize(jv);
        outFile << jsonString << std::endl;
        outFile.close();
    }
}


