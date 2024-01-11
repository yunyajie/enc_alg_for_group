#include "server.h"


int main(){
    Server server(8000, 0, "root", "@Lab510510", "ph_cipher", 10, 30, 5000);
    server.start();
    return 0;
}