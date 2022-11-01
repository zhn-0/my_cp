#include "crypto.hpp"

unsigned char myAesKey[MD5_DIGEST_LENGTH];

int generateAesKey(unsigned char *pwd)
{
    MD5(pwd, strlen((char *)pwd), myAesKey);
    return 0;
}

int encrypt(int srcFd, int dstFd)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);

    unsigned char iv[16];
    memset(iv, 0x1, sizeof(iv));
    EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, myAesKey, iv);

    int n, outlen;
    unsigned char bufIn[16];
    unsigned char bufOut[64];
    while ((n = read(srcFd, bufIn, 16)) > 0)
    {
        if (!EVP_EncryptUpdate(ctx, bufOut, &outlen, bufIn, n))
        {
            EVP_CIPHER_CTX_free(ctx);
            perror("Encrypt update");
            return 1;
        }
        write(dstFd, bufOut, outlen);
    }

    int tmplen = 0;
    if (!EVP_EncryptFinal_ex(ctx, bufOut + outlen, &tmplen))
    {
        EVP_CIPHER_CTX_free(ctx);
        return 1;
    }

    write(dstFd, bufOut + outlen, tmplen);

    EVP_CIPHER_CTX_free(ctx);

    return 0;
}

int decrypt(int srcFd, int dstFd)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX_init(ctx);

    unsigned char iv[16];
    memset(iv, 0x1, sizeof(iv));
    EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, myAesKey, iv);

    int n, outlen;
    unsigned char bufIn[16];
    unsigned char bufOut[64];
    while ((n = read(srcFd, bufIn, 16)) > 0)
    {
        if (!EVP_DecryptUpdate(ctx, bufOut, &outlen, bufIn, n))
        {
            EVP_CIPHER_CTX_free(ctx);
            perror("Decrypt update");
            return 1;
        }
        write(dstFd, bufOut, outlen);
    }

    int tmplen = 0;
    if (!EVP_DecryptFinal_ex(ctx, bufOut + outlen, &tmplen))
    {
        EVP_CIPHER_CTX_free(ctx);
        return 1;
    }

    write(dstFd, bufOut + outlen, tmplen);

    EVP_CIPHER_CTX_free(ctx);

    return 0;
}