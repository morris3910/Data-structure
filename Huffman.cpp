//謝鎧駿 B103040021 
//2022/12/6
//製作 Huffman壓縮軟體
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <vector>

using namespace std;

vector<unsigned char> RawFile;//暫存input file的資料 
vector<unsigned char> Result;//result
map<unsigned char, int> FreqPerChar; // 用來統計各個字元的出現次數
map<unsigned char, string> HuffmanTable;//Huffman Table
map<string, unsigned char> DecodeTable;//Decoding Table
int totalbit = 0;

struct HuffmanNode {
    HuffmanNode* LC = NULL;
    HuffmanNode* RC = NULL;
    int freq;
    unsigned char ch;
    HuffmanNode(unsigned char c, int f) {
        ch = c;
        freq = f;
    }
};

//sorting比較函式
struct cmp {
    bool operator()(HuffmanNode* n1, HuffmanNode* n2) {
        //次數相等則字典序小的優先取出
        if (n1->freq == n2->freq) {
            return n1->ch > n2->ch;
        }
        return n1->freq > n2->freq;
    }
};

//尋找葉子 製作編碼表
void DFS(string Code, HuffmanNode* nowNode) {
    if (nowNode->LC == NULL && nowNode->RC == NULL) {
        HuffmanTable.emplace(make_pair(nowNode->ch, Code));
        return;
    }
    DFS(Code + '0', nowNode->LC);
    DFS(Code + '1', nowNode->RC);
}

//進行編碼 每8個bit轉成unsigned char
void Encode() {
    int cnt = 0;
    unsigned char tmp = 0;
    for (auto i : RawFile) {
        for (int j = 0; j < HuffmanTable[i].size(); j++) {
            totalbit++;
            tmp <<= 1;
            cnt++;
            tmp += (HuffmanTable[i][j] - '0');
            if (cnt == 8) {
                cnt = 0;
                Result.emplace_back(tmp);
                tmp = 0;
            }
        }
    }
    if (cnt) Result.emplace_back(tmp);
}

//解碼並輸出結果
void Decode(string FileName) {
    fstream OutFile;
    OutFile.open(FileName, ios::out | ios::binary);
    string buff = "";
    int Prefix = (totalbit / 8) * 8, bitcnt = 0;
    for (auto i : RawFile) {
        for (int j = 7; j >= 0; j--) {
            bitcnt++;

            //未完整的byte處理
            if (bitcnt > Prefix && bitcnt <= Prefix + (8 - totalbit % 8))
                continue;

            // char to bool
            buff += (((i >> j) & 1) + '0');

            if (DecodeTable.count(buff)) {
                OutFile << DecodeTable[buff];
                buff = "";
            }
        }
    }
    OutFile.close();
}

//讀入要壓縮的檔案
void ReadFile(string FileName) {
    ifstream InputFile;
    InputFile.open(FileName, ios::in | ios::binary);

    unsigned char ch = InputFile.get();
    while (InputFile.good()) {
        RawFile.emplace_back(ch);

        //儲存次數
        FreqPerChar[ch]++;

        ch = InputFile.get();
    }
    InputFile.close();
}

//讀入壓縮過的檔案
void ReadBinFile(string FileName) {
    ifstream InputFile;
    InputFile.open(FileName, ios::in | ios::binary);
    int n = 0;

    int buffC;
    string buffS;

    // n個編碼 讀入totalbit以進行未完整的byte處理
    InputFile >> n >> totalbit;

    for (int i = 0; i < n; i++) {
        InputFile >> buffC >> buffS;
        DecodeTable[buffS] = buffC;
    }

    //壓縮前byte,壓縮後byte,壓縮率
    string Before, After, compressRatio;
    InputFile >> Before >> After >> compressRatio;
    InputFile.get();
    unsigned char ch = InputFile.get();
    while (InputFile.good()) {
        RawFile.emplace_back(ch);
        ch = InputFile.get();
    }
    InputFile.close();
}

HuffmanNode* BuildHuffmanTree() {
    priority_queue<HuffmanNode*, vector<HuffmanNode*>, cmp> pq;
    for (auto i : FreqPerChar) {
        pq.push(new HuffmanNode(i.first, i.second));
    }
    while (pq.size() != 1) {
        HuffmanNode *n1, *n2, *tmp;
        n1 = pq.top();
        pq.pop();
        n2 = pq.top();
        pq.pop();
        unsigned char cmin = min(n1->ch, n2->ch);
        tmp = new HuffmanNode(cmin, n1->freq + n2->freq);
        if (cmin == n1->ch) {
            tmp->LC = n1;
            tmp->RC = n2;
        } else {
            tmp->RC = n1;
            tmp->LC = n2;
        }
        pq.push(tmp);
    }
    return pq.top();
}

void GenZipFile(string FileName) {
    fstream OutFile;
    OutFile.open(FileName, ios::out | ios::binary);
    OutFile << HuffmanTable.size() << ' ' << totalbit << '\n';

    //輸出編碼表
    for (auto i : HuffmanTable) {
        cout << i.first << ": " << i.second << '\n';
        OutFile << (int)i.first << ' ' << i.second << '\n';
    }

    //計算並輸出壓縮率及壓縮前後的byte
    OutFile << RawFile.size() << '\n';
    int Headersize = 6 + OutFile.tellg();
    int digit = 0, temp = Headersize + Result.size();
    while (temp) {
        temp /= 10;
        digit++;
    }
    int CompressedSize = Result.size() + Headersize + digit + 1;
    OutFile << CompressedSize << '\n';
    OutFile << fixed << setprecision(3) << 1.0 * RawFile.size() / CompressedSize
            << '\n';

    cout << "Origin:" << RawFile.size() << "Bytes\n";
    cout << "Compressed:" << CompressedSize << "Bytes\n";
    cout << "Compression Ratio:" << 1.0 * RawFile.size() / CompressedSize
         << '\n';

    for (auto i : Result) {
        OutFile << i;
    }
    OutFile.close();
}

void Zip(string InputFileName, string OutputFileName) {
    ReadFile(InputFileName);
    HuffmanNode* root = BuildHuffmanTree();
    DFS("", root);
    Encode();
    GenZipFile(OutputFileName);
}

void Unzip(string InputFileName, string OutputFileName) {
    ReadBinFile(InputFileName);
    Decode(OutputFileName);
}

int main(int argc, char* argv[]) {
    if (argc == 6) {
        if (string(argv[1]) == "-c") {
            Zip(string(argv[3]), string(argv[5]));
        } else if (string(argv[1]) == "-u") {
            Unzip(string(argv[3]), string(argv[5]));
        } else {
            printf("ERROR1\n");
        }
    } else {
        printf("ERROR2\n");
    }
    return 0;
}
