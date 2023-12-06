#include <iostream>
#include <gmpxx.h>
#include "PH_Cipher.h"
#include <vector>

int main(){
    //初始化一个容量为 10 的系统
    PH_Cipher ph_cipher(10, 32);
    ph_cipher.init();
    ph_cipher.test();
    return 0;
}