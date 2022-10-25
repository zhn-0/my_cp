#include "Huffman.hpp"

void HuffmanTree::freeHuffmanNode(HuffmanNode *cur)
{ //析构函数使用
    if(cur->lc)freeHuffmanNode(cur->lc);
    if(cur->rc)freeHuffmanNode(cur->rc);
    delete cur;
}

void HuffmanTree::traverseHuffmanTree(HuffmanNode *cur, std::string s)
{ //生成压缩码
    if (cur->isLeaf)
    {
        code[cur->c] = s;
        return;
    }
    if (cur->lc)
    {
        traverseHuffmanTree(cur->lc, s + "0");
    }
    if (cur->rc)
    {
        traverseHuffmanTree(cur->rc, s + "1");
    }
}

int HuffmanTree::createTree()
{ //根据频率生成哈夫曼树
    auto cmp = [](std::pair<HuffmanNode*, int>&a, std::pair<HuffmanNode*, int>&b){
        return a.second > b.second || a.second == b.second && a.first->isLeaf == true;
    };

    std::priority_queue<std::pair<HuffmanNode*, int>, std::vector<std::pair<HuffmanNode*, int>>, decltype(cmp)> q(cmp);

    for (int i = 0; i < BYTESIZE; ++i)
    {
        if(freq[i] > 0)
        {
            auto p = new HuffmanNode(i);
            q.push({p, freq[i]});
        }
    }

    while(q.size() > 1)
    {
        auto [p1, val1] = q.top();
        q.pop();
        auto [p2, val2] = q.top();
        q.pop();
        auto p = new HuffmanNode();
        p->lc = p1;
        p->rc = p2;
        q.push({p, val1 + val2});
    }

    root = q.top().first;

    traverseHuffmanTree(root, "");

    return 0;
}

int compress(int srcFd, int dstFd)
{
    unsigned char c;
    HuffmanTree t;
    while(read(srcFd, &c, sizeof(c)) > 0)
    {
        ++t.freq[c];
    }
    t.createTree();
    int ret=0;
    if((ret = write(dstFd, t.freq, sizeof(t.freq)))<0)
    {
        perror("write");
        return -1;
    }

    lseek(srcFd, 0, SEEK_SET);
    int bitcount = 0; //记录是否满8位
    unsigned char onebyte = 0; //要写入的字节
    while(read(srcFd, &c, sizeof(c)) > 0)
    {
        std::string s = t.code[c];
        for(char &ch : s)
        {
            onebyte <<= 1;
            if(ch == '1')
            {
                onebyte |= 1;
            }
            ++bitcount;
            if(bitcount == 8)
            {
                write(dstFd, &onebyte, sizeof(onebyte));
                bitcount = 0;
                onebyte = 0;
                ++ret;
            }
        }
    }

    if(bitcount > 0)
    {
        onebyte <<= (8 - bitcount);
        write(dstFd, &onebyte, sizeof(onebyte));
        ++ret;
    }

    return ret;
}

int uncompress(int srcFd, int dstFd, size_t size)
{
    unsigned char c;
    HuffmanTree t;
    size_t readNum = 0; // 文件初始有freq数组
    if((readNum = read(srcFd, t.freq, sizeof(t.freq))) < 0)
    {
        perror("read");
        return -1;
    }
    t.createTree();

    HuffmanNode *p = t.root;
    while (readNum < size)
    {
        read(srcFd, &c, sizeof(c));
        for (int i = 7; i >= 0; --i)
        {
            if (c & (1 << i))
            {
                p = p->rc;
            }
            else
            {
                p = p->lc;
            }
            if (p->isLeaf)
            {
                write(dstFd, &p->c, sizeof(p->c));
                p = t.root;
            }
        }
        ++readNum;
    }

    return 0;
}