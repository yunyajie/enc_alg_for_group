#include "PH_Cipher.h"

//PH_Member 的实现

PH_Member::PH_Member(mpz_class enc_key, mpz_class modulus):enc_key(enc_key), modulus(modulus){
    //初始化解密密钥
    mpz_invert(dec_key.get_mpz_t(), enc_key.get_mpz_t(), static_cast<mpz_class>(modulus - 1).get_mpz_t());
}

mpz_class PH_Member::decrypt(mpz_class& ciphertext){
    mpz_class message;
    mpz_powm(message.get_mpz_t(), ciphertext.get_mpz_t(), dec_key.get_mpz_t(), modulus.get_mpz_t());
    return message;
}





//PH_Cipher 的实现

mpz_class PH_Cipher::encrypt(mpz_class& message){
    mpz_class cipher_text;
    mpz_powm(cipher_text.get_mpz_t(), message.get_mpz_t(), m_key.get_mpz_t(), active_mod_product.get_mpz_t());
    return cipher_text;
}

int PH_Cipher::member_join(PH_Member joiner){

    return 0;
}

int PH_Cipher::member_leave(PH_Member leaver){
    
    return 0;
}