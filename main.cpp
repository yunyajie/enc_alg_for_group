#include "server/server.h"


int main(){
    Server server(8000);
    server.start();
    return 0;
}