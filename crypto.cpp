#include "crypto.hpp"

unsigned char myAesKey[MD5_DIGEST_LENGTH];

int generateAesKey(unsigned char *pwd)
{
    MD5(pwd, strlen((char *)pwd), myAesKey);
    return 0;
}

int encrypt(int srcFd, int dstFd)
{
    write(dstFd, myAesKey, MD5_DIGEST_LENGTH);

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
            fprintf(stderr, "Encrypt update failed\n");
            exit(EXIT_FAILURE);
        }
        write(dstFd, bufOut, outlen);
    }

    int tmplen = 0;
    if (!EVP_EncryptFinal_ex(ctx, bufOut + outlen, &tmplen))
    {
        EVP_CIPHER_CTX_free(ctx);
        fprintf(stderr, "Encrypt final failed\n");
        exit(EXIT_FAILURE);
    }

    write(dstFd, bufOut + outlen, tmplen);

    EVP_CIPHER_CTX_free(ctx);

    return 0;
}

int decrypt(int srcFd, int dstFd)
{
    unsigned char pwd[MD5_DIGEST_LENGTH];
    read(srcFd, pwd, MD5_DIGEST_LENGTH);
    for(int i=0;i<MD5_DIGEST_LENGTH;++i)
    {
        if(pwd[i]!=myAesKey[i])
        {
            fprintf(stderr, "Wrong Password\n");
            unlink(".tmpfile_for_decrypto");
            exit(EXIT_FAILURE);
        }
    }
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
            fprintf(stderr, "Decrypt update failed\n");
            exit(EXIT_FAILURE);
        }
        write(dstFd, bufOut, outlen);
    }

    int tmplen = 0;
    if (!EVP_DecryptFinal_ex(ctx, bufOut + outlen, &tmplen))
    {
        EVP_CIPHER_CTX_free(ctx);
        fprintf(stderr, "Decrypt final failed\n");
        exit(EXIT_FAILURE);
    }

    write(dstFd, bufOut + outlen, tmplen);

    EVP_CIPHER_CTX_free(ctx);

    return 0;
}