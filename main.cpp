#include <iostream>
#include <gmpxx.h>
#include "mylib.h"
#include <vector>

int main(){
    mpz_class safe_prime;
    //生成 10 个 1024 位的安全素数
    for(int i = 0; i < 10; i++){
        generate_safe_prime(safe_prime, 1024, 25);
        std::cout << "第 " << i + 1 << " 个 1024 位的安全素数 ：" << safe_prime << std::endl;
    }
    //生成 10 个 2048 位的安全素数
    for(int i = 0; i < 10; i++){
        generate_safe_prime(safe_prime, 2048, 25);
        std::cout << "第 " << i + 1 << " 个 2048 位的安全素数 ：" << safe_prime << std::endl;
    }
    return 0;
}