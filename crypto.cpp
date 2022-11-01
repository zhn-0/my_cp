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

    int n, tol;
    unsigned char buf[16];
    unsigned char to[64];
    while ((n = read(srcFd, buf, 16)) > 0)
    {
        if (!EVP_EncryptUpdate(ctx, to, &tol, buf, n))
        {
            EVP_CIPHER_CTX_free(ctx);
            perror("Encrypt update");
            return 1;
        }
        write(dstFd, to, tol);
    }

    int tmplen = 0;
    if (!EVP_EncryptFinal_ex(ctx, to + tol, &tmplen))
    {
        EVP_CIPHER_CTX_free(ctx);
        return 1;
    }

    write(dstFd, to + tol, tmplen);

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

    int n, tol;
    unsigned char buf[16];
    unsigned char to[64];
    while ((n = read(srcFd, buf, 16)) > 0)
    {
        if (!EVP_DecryptUpdate(ctx, to, &tol, buf, n))
        {
            EVP_CIPHER_CTX_free(ctx);
            perror("Decrypt update");
            return 1;
        }
        write(dstFd, to, tol);
    }

    int tmplen = 0;
    if (!EVP_DecryptFinal_ex(ctx, to + tol, &tmplen))
    {
        EVP_CIPHER_CTX_free(ctx);
        return 1;
    }

    write(dstFd, to + tol, tmplen);

    EVP_CIPHER_CTX_free(ctx);

    return 0;
}