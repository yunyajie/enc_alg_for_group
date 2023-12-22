# enc_alg_for_group
//在 new_ph 分支上重构一下 PH_Cipher
g++ -o main main.cpp server/server.cpp PH_Cipher/PH_Cipher.cpp conn/conn.cpp buffer/buffer.cpp server/epoller.cpp -lgmp -lgmpxx
g++ -o cli_test client/client.cpp client/main.cpp PH_Cipher/PH_Cipher.cpp -lgmp -lgmpxx