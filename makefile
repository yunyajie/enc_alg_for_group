main: PH_Cipher.cpp main.cpp
	g++ -o main PH_Cipher.cpp main.cpp -lgmp -lgmpxx
clean:
	rm main