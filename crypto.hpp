#ifndef CRYPTO
#define CRYPTO

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/md5.h>

int generateAesKey(unsigned char *pwd);
int encrypt(int, int);
int decrypt(int, int);

#endif