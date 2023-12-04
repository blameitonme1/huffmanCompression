#include<bits/stdc++.h>
using namespace std;
// haffman tree 节点.
class HuffmanNode {
public:
    char data; // 数据本身
    int frequency; // 字符的frequency
    HuffmanNode *left, *right;

    HuffmanNode(char data, int frequency) : data(data), frequency(frequency), left(nullptr), right(nullptr) {}
    // 重载使用优先队列
    bool operator > (const HuffmanNode& other) const {
        return frequency > other.frequency;
    }
};
// 根据哈希表构建huffman tree
HuffmanNode* buildHuffmanTree(const unordered_map<char, int>& freqs) {
    priority_queue<HuffmanNode, vector<HuffmanNode>, greater<HuffmanNode>> minHeap;

    // 将字符频率映射加入优先队列
    for (const auto& entry : freqs) {
        minHeap.push(HuffmanNode(entry.first, entry.second));
    }

    // 通过合并节点构建哈夫曼树
    while (minHeap.size() > 1) {
        HuffmanNode* left = new HuffmanNode('\0', 0);
        *left = minHeap.top();
        minHeap.pop();

        HuffmanNode* right = new HuffmanNode('\0', 0);
        *right = minHeap.top();
        minHeap.pop();
        // 合并之后加入到优先队列里面
        HuffmanNode* merged = new HuffmanNode('\0', left->frequency + right->frequency);
        merged->left = left;
        merged->right = right;
        minHeap.push(*merged);
    }
    HuffmanNode* res = new HuffmanNode(minHeap.top().data, minHeap.top().frequency);
    res->left = minHeap.top().left;
    res->right = minHeap.top().right;
    return res;
}
// 生成哈夫曼编码表
void buildHuffmanCodes(HuffmanNode* root, string code, unordered_map<char, string>& huffmanCodes) {
    if (root) {
        if (root->data != '\0') {
            huffmanCodes[root->data] = code;
        }
        buildHuffmanCodes(root->left, code + "0", huffmanCodes);
        buildHuffmanCodes(root->right, code + "1", huffmanCodes);
    }
}

// 压缩文件
void compressFile(const string& inputFile, const string& outputFile) {
    // 统计字符频率
    unordered_map<char, int> freqs;
    ifstream input(inputFile);
    char ch;
    while (input.get(ch)) {
        freqs[ch]++;
    }
    input.close();

    // 构建哈夫曼树
    HuffmanNode* root = buildHuffmanTree(freqs);

    // 生成哈夫曼编码表，上面定义了相关函数
    unordered_map<char, string> huffmanCodes;
    buildHuffmanCodes(root, "", huffmanCodes);
    // 压缩文件,注意使用二进制文件输出
    input.open(inputFile);
    ofstream output(outputFile, ios::binary);

    // 写入编码表大小，注意write要传入const char*的指针
    size_t numCodes = huffmanCodes.size();
    output.write(reinterpret_cast<const char*>(&numCodes), sizeof(numCodes));

    // 写入编码表，方便之后还原的时候对应编码表还原
    for (const auto& entry : huffmanCodes) {
        output.put(entry.first);
        size_t codeSize = entry.second.size();
        // 这个地方写入字符串的大小，方便解压缩的时候重建huffmanCodes
        output.write(reinterpret_cast<const char*>(&codeSize), sizeof(codeSize));
        output.write(entry.second.c_str(), codeSize);
    }

    // 写入压缩数据
    bitset<8> buffer;
    size_t index = 0;
    while (input.get(ch)) {
       //  cout<<"char is"<<ch<<" code is: "<<huffmanCodes[ch]<<endl;
        string code = huffmanCodes[ch];
        for (char bit : code) {
            // 注意从高位开始设置
            buffer.set(7 - index++, bit - '0');
            if (index == 8) {
                // 此时buffer已经装满了，将buffer转化为long再转化为字符串
                // cout<<static_cast<char>(buffer.to_ulong())<<endl;
                output.put(static_cast<char>(buffer.to_ulong()));
                buffer.reset();
                index = 0;
            }
        }
    }

    // 处理最后的不足8位的部分，因为不是没个编码都能填满8个位置
    if (index > 0) {
        output.put(static_cast<char>(buffer.to_ulong()));
    }

    input.close();
    output.close();
}

// 根据压缩文件里面的编码表重建哈夫曼树
HuffmanNode* rebuildHuffmanTree(const unordered_map<char, string>& huffmanCodes) {
    HuffmanNode* root = new HuffmanNode('\0', 0);
    for (const auto& entry : huffmanCodes) {
        // cout<<entry.first<<' '<<entry.second<<endl;
        HuffmanNode* current = root;
        for (char bit : entry.second) {
            if (bit == '0') {
                if (!current->left) {
                    current->left = new HuffmanNode('\0', 0);
                }
                current = current->left;
            } else if (bit == '1') {
                if (!current->right) {
                    current->right = new HuffmanNode('\0', 0);
                }
                current = current->right;
            }
        }
        current->data = entry.first;
    }
    return root;
}
// 解压缩文件
void decompressFile(const string& inputFile, const string& outputFile) {
    // 因为这个时候是读取压缩文件，所以打开二进制
    ifstream input(inputFile, ios::binary);
    ofstream output(outputFile);

    // 读取编码表大小
    size_t numCodes;
    input.read(reinterpret_cast<char*>(&numCodes), sizeof(numCodes));

    // 读取编码表
    unordered_map<char, string> huffmanCodes;
    for (size_t i = 0; i < numCodes; ++i) {
        char ch;
        size_t codeSize;
        input.get(ch);
        input.read(reinterpret_cast<char*>(&codeSize), sizeof(codeSize));
        string code;
        code.resize(codeSize);
        input.read(&code[0], codeSize);
        huffmanCodes[ch] = code;
        // cout<<code<<endl;
    }
    // cout<<huffmanCodes.size()<<endl;
    // 重建哈夫曼树
    HuffmanNode* root = rebuildHuffmanTree(huffmanCodes);

    // 解码数据
    HuffmanNode* current = root;
    char byte;
    while (input.get(byte)) {
            for (int i = 7; i >= 0; --i) {
                // 从高位到低位依次获取每个比特,这里记得要读取每一位，加上括号防止优先级问题
                char bit = ((byte >> i) & 1) + '0';
                // cout<<bit<<endl;
                if (bit == '0') {
                current = current->left;
            } else if (bit == '1') {
                current = current->right;
            }

            if (current->left == nullptr && current->right == nullptr) {
                // 当前节点为叶子节点，表示找到一个字符，写入到输出文件
                // cout<<"data is "<<current->data<<endl;
                output.put(current->data);
                current = root;  // 重置当前节点，重新开始
            }
        }
    }

    input.close();
    output.close();
}
int main() {
    const string inputFileName = "input.txt";
    const string compressedFileName = "compressed.huff";
    const string decompressedFileName = "decompressed.txt";

    // // 创建一个测试文件
    // ofstream inputFile(inputFileName);
    // inputFile << "Hello, this is a test string for Huffman coding!safhiasufghasuifgchasudgasuifgsadgyusadgasygfgsgfggsafigsifgsfugsufsufgsufgusfgusfgusagfiasfgusadhhhhhhhhhhhhhhhhhsdfghufgeyuqwfgiuygfudsgfsjfgsauifasiufsafhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh";
    // inputFile.close();

    // 压缩文件
    compressFile(inputFileName, compressedFileName);
    uintmax_t firstSize = std::filesystem::file_size(inputFileName);
    uintmax_t afterSize = std::filesystem::file_size(compressedFileName);
    cout<<"compress rate: "<<(double)afterSize / (double)firstSize<<endl;
    // 解压缩文件
    decompressFile(compressedFileName, decompressedFileName);
    getchar();
    return 0;
}
