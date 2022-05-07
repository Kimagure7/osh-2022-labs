// IO
#include <iostream>
// std::string
#include <string>
#include <cstring>
// std::vector
#include <vector>
// std::string 转 int
#include <sstream>
// PATH_MAX 等常量
#include <climits>
// POSIX API
#include <unistd.h>
// wait
#include <sys/wait.h>
// 重定向用open
#include <fcntl.h>
// string error
#include <errno.h>
// signal
#include <signal.h>
// 文件读写用open
#include <fstream>
// setw
#include <iomanip>
// system函数
#include <stdlib.h>
// sigjump signla
#include <setjmp.h>
using namespace std;

#define WRITE_END 1       // pipe write end
#define READ_END 0        // pipe read end
#define CMD_MAX 32        // cmd最大命令数
#define Code_NoFileName 2 //错误代码
#define FAIL_TO_OPEN 3    //无法打开文件

class history
{
private:
    // history文件名
    const char *Filename = ".bash_history";
    int origin_size;

public:
    vector<string> hist;
    history() {}
    ~history() {}

    void read();
    void write();
    //显示所有histrory
    void show(int pos = 0)
    {
        if (!hist.size())
        {
            cout << "history file empty" << endl;
            return;
        }
        if (!pos)
        {
            pos = hist.size();
        }
        for (int i = hist.size() - pos; i < hist.size(); i++)
        {
            cout << i << " " << hist[i] << endl;
        }
    }
    inline int get_origin_size() { return origin_size; }
};

//从Filename中读取history 格式为vector<string>
void history::read()
{
    ifstream fin;
    fin.open(Filename, ios_base::in);
    if (!fin.is_open())
    {
        cout << "no history file \".bash_history\" is loaded" << endl;
        fin.close();
        origin_size = 0;
        return;
    }
    string temp;
    while (true)
    {
        getline(fin, temp);
        if (temp == "")
            break; //读完了
        hist.push_back(temp);
    }
    fin.close();
    origin_size = hist.size();
    return;
}

//用追加模式写入history 提高性能
void history::write()
{
    std::ofstream fout;
    fout.open(Filename, ios_base::app);
    if (!fout.is_open())
    {
        cout << "failed to save history file" << endl;
        exit(FAIL_TO_OPEN);
    }
    for (int i = origin_size; i < hist.size(); i++)
    {
        fout << hist[i] << endl;
    }
    fout.close();
    // cout<<"写入成功"<<endl;
    return;
}

// 经典的 cpp string split 实现
// https://stackoverflow.com/a/14266139/11691878
std::vector<std::string> split(std::string s, const std::string &delimiter)
{
    std::vector<std::string> res;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos)
    {
        token = s.substr(0, pos);
        res.push_back(token);
        s = s.substr(pos + delimiter.length());
    }
    res.push_back(s);
    return res;
}