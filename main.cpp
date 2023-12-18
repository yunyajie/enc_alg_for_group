#include <iostream>
#include <vector>
#include "PH_Cipher/PH_Cipher.h"
#include "server/epoller.h"
#include "server/server.h"
#include "buffer/buffer.h"
#include <arpa/inet.h> // sockaddr_in
#include <fcntl.h>


int main(){
    Server server(8000);
    server.start();
    return 0;
}