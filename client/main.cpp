#include "client.h"
int main(){
    Client client(8000, "172.18.0.1");
    client.test();
    return 0;
}