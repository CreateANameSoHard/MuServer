#pragma once
#include <sys/socket.h>
#include<arpa/inet.h>
#include<string>
#include <strings.h>

#include "copyable.h"

class InetAddress:copyable
{
public:
   explicit InetAddress(uint16_t port = 8080,std::string ip = "127.0.0.1");
   explicit InetAddress(const struct sockaddr_in& addr)
   :addr_(addr)
   {}

   std::string toIp() const;
   std::string toIpPort() const;
   uint16_t toPort() const;

   const struct sockaddr_in* getSockAddr() const
   {
      return &addr_;
   }

   void setSockAddr(const sockaddr_in& addr){addr_ = addr;}
private:
    struct sockaddr_in addr_;
};