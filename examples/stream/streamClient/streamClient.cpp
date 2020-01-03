
#include <iostream>
#include <stream/StreamClient.hpp>
#include <stream/DataSocket.hpp>
#include <HostName.h>
#include <algorithm>

int main(int argc, char *argv[])
{
    uint8_t buffer [128];
    
    auto baseSocket = EtNet::CBaseSocket(EtNet::ESocketMode::INET6_STREAM);
    EtNet::CStreamClient streamClient(std::move(baseSocket));
//    EtNet::CDataSocket dataSocket = streamClient.connect("192.168.1.22",5001);
    EtNet::CDataSocket dataSocket = streamClient.connect("tom-fujitsu",5001);

    std::string test ("Hallo");
    dataSocket.send(test.c_str(),test.length());
    int rcvLen= dataSocket.recive(buffer, sizeof(buffer), [](std::size_t len) { 
         std::cout << "Rcv Len:" << len << std::endl;;
         return true;
    });
    std::cout << "Rcv: " << (char*)buffer << std::endl;
}
