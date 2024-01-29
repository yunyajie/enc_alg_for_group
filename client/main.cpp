#include "client.h"
int main(int argc, char* argv[]){
    if(argc < 3){
        std::cout << "expect: " << argv[0] << " user_name passwd" << std::endl;
        return -1;
    }
    Client client(8000, "192.168.194.128", argv[1], argv[2]);
    client.start();
    return 0;
}