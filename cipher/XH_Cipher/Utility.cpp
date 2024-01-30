#include "Utility.h"

int sha256encrypt(vector<string>& strs, mpz_class& result){
    if(strs.empty()) return -1;
    unsigned char* md[SHA256_DIGEST_LENGTH]
    SHA256_CTX ctx;
    SHA256_Init(&ctx);  //初始化指针
    for(string s : strs){
        SHA256_Update(&ctx, s.c_str(), s.size());
    }
    SHA256_Final(md, &ctx); //生成最终的加密哈希
    string t = "";
    char buf[2];
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++){
        sprintf(buf, "%02x", md[i]);
        t += buf;
    }
    result = mpz_class(t, 16);
    return 0;
}