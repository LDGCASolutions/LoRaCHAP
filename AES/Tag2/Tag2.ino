#include <Crypto.h>
#include <AES.h>
#include "base64.hpp"

AES256 aes256;

byte buffer[16];
byte buffer2[16];
byte base64_text[20];
byte decoded_text[20];

void setup() {
  Serial.begin(115200);
  
  BlockCipher *cipher = &aes256;

  // Planetext
  char message[20] = "hello world!";

  byte key[33] = "12345678123456781234567812345678";
  
  // Encrypt AES-256-ECB
  crypto_feed_watchdog();
  cipher->setKey(key, 32);
  cipher->encryptBlock(buffer, message);
  Serial.print("Original: ");
  Serial.println(message);
    
  
  // Base64 encode
  int base64_length = encode_base64(buffer,16,base64_text);
  Serial.print("Base64 Text: ");Serial.println((char *) base64_text);
  Serial.print("Base64 Length: ");Serial.println(base64_length);

  
  // Base64 decode
  int decoded_length = decode_base64(base64_text,decoded_text);
  Serial.print("Decoded Text: ");Serial.println((char *)decoded_text);
  Serial.print("Decoded Length: ");Serial.println(decoded_length);
  

  // Decrypt AES-256-ECB
  cipher->setKey(key, 32);
  cipher->decryptBlock(buffer2, decoded_text);
  Serial.print("Output: ");
  Serial.println((char*)buffer2);
  
}

void loop() {
}
