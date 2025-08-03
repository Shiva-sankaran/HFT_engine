#include<string>
#include<netinet/in.h>

struct MarketFeed
{
    std::string name;
    int port;
    std::string ip;

    MarketFeed(std::string name, int port, std::string ip): name(name),port(port),ip(ip) {}

};
