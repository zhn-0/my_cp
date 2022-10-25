#ifndef HUFFMAN
#define HUFFMAN

#include <queue>
#include <vector>
#include <string>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#define BYTESIZE 256

class HuffmanNode
{
public:
    bool isLeaf; //是否为根节点
    unsigned char c; //压缩的字节
    HuffmanNode *lc, *rc; //左右子节点

    HuffmanNode() : isLeaf(false), c(-1), lc(nullptr), rc(nullptr) {}
    HuffmanNode(int c) : isLeaf(true), c(c), lc(nullptr), rc(nullptr) {}
};

class HuffmanTree
{
private:
    void freeHuffmanNode(HuffmanNode *cur);
    void traverseHuffmanTree(HuffmanNode *cur, std::string s);
    
public:
    HuffmanNode *root; //树的根节点
    int freq[BYTESIZE]; //统计频率
    std::string code[BYTESIZE]; //压缩码

    HuffmanTree() : root(nullptr) {  memset(freq, 0, sizeof(freq));  }
    ~HuffmanTree() {  freeHuffmanNode(root);  }
    int createTree();
};

int compress(int srcFd, int dstFd);
int uncompress(int srcFd, int dstFd, size_t size);

#endif