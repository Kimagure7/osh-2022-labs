#include "shell.h"

using namespace std;

int cmd_builtin(string cmd);
int cmd_out_p(string cmd);
int cmd_redirect(std::vector<std::string> &agrs, int *fd);
void signal_callback(int signum);

history his;
jmp_buf buf;

int main()
{
    // 不同步 iostream 和 cstdio 的 buffer
    // 这样会少点ctrl + c 的bug
    // std::ios::sync_with_stdio(false);
    his.read();
    // his.show();
    signal(SIGINT, signal_callback);
    std::string cmd;
    while (true)
    {
        // 打印提示符
        // system("echo pwd");
        std::string cwd_default; //用于提示符
        cwd_default.resize(PATH_MAX);
        const char *ret = getcwd(&cwd_default[0], PATH_MAX);
        if (ret == nullptr)
        {
            std::cout << "default cwd failed\n";
        }
        else
        {
            std::cout << ret << "$ ";
        }

        int key;
        //终端从一次一行变成一次一字符 相当于实时监控
        system("stty raw");
        //方向键选择history 青春版 可以上下 可以backspace 左右懒得写 比较麻烦
        //表示查看的历史的位置
        //直接对应hist 下标 -1表示还没开始查
        int his_pos = -1;

        while (true)
        {
            key = getchar();
            cmd += (char)key; //记录当前命令行数据
            // enter
            if (key == 13)
            {
                //^M 回退两个字符
                // \b是移动光标 不是删除字符 所以要在输出空格再移回去
                cout << "\b\b  \b\b";
                system("stty -raw"); //先取消 不然endl有bug
                cmd.pop_back();
                if (cmd.size() == 0)
                { //空命令
                    std::cout << std::endl;
                    if (ret == nullptr)
                    {
                        std::cout << "getcwd failed"
                                  << "$ ";
                    }
                    else
                    {
                        std::cout << ret << "$ ";
                    }
                    fflush(stdout);
                    system("stty raw");
                    continue;
                }
                else
                {
                    cout << endl;
                    //直接去执行下面的命令
                    break;
                }
            }
            // ctrl + c
            if (key == 3)
            {
                // ^C显示出来也挺好 不管他
                system("stty -raw");
                cmd.clear(); //清空
                std::cout << std::endl;
                if (ret == nullptr)
                {
                    std::cout << "getcwd failed"
                              << "$ ";
                }
                else
                {
                    std::cout << ret << "$ ";
                }
                fflush(stdout);
                system("stty raw");
                continue;
            }
            // backspace
            if (key == 127)
            {
                //多退一个 因为是删除
                cout << "\b\b\b   \b\b\b";
                cmd.pop_back(); //抛弃backspace
                cmd.pop_back();
                fflush(stdout);
                continue;
            }
            // EOF
            if (key == 4)
            {
                system("stty -raw");
                cout << endl;
                cmd = "exit";
                break;
            }
            // 上下左右方向键
            // 上键相当于三个字符，对应 ascii 应该是 27、91、65，下键是 27、91、66
            // 左是 27、91、68，右是 27、91、67
            if (key == 27 && (key = getchar()) == 91)
            {
                key = getchar();
                cmd.pop_back(); //抛弃一个字符
                if (key == 65)
                {

                    if ((his.hist.size() == 0) || (!his_pos)) //查到头了 或者还没有history
                    {
                        cout << "\b\b\b\b    \b\b\b\b";
                        // 也就是没得再输出了，保持原状, 但现在已经输出了 ^[[A
                        fflush(stdout);
                        continue;
                    }
                    else
                    {
                        // 能输出上一条，进入 history
                        printf("\33[2K\r"); //直接清行 消除原来所有字符
                        if (ret == nullptr)
                        {
                            std::cout << "getcwd failed"
                                      << "$ ";
                        }
                        else
                        {
                            std::cout << ret << "$ ";
                        }
                        if (his_pos == -1)
                        {
                            cmd = his.hist[his.hist.size() - 1];
                            his_pos = his.hist.size() - 1;
                        }
                        else
                        {
                            cmd = his.hist[--his_pos];
                        }
                        std::cout << cmd;
                        fflush(stdout);

                        continue;
                    }
                }
                else if (key == 66)
                {
                    if ((his_pos == -1) || (his.hist.size() == his_pos)) //还没开始看 或者 最后一个了
                    {
                        cout << "\b\b\b\b    \b\b\b\b";
                        // 也就是没得再输出了，保持原状, 但现在已经输出了 ^[[A
                        fflush(stdout);
                        continue;
                    }
                    else
                    {
                        // 能输出下一条，进入 history
                        printf("\33[2K\r"); //直接清行 消除原来所有字符
                        if (ret == nullptr)
                        {
                            std::cout << "getcwd failed"
                                      << "$ ";
                        }
                        else
                        {
                            std::cout << ret << "$ ";
                        }
                        cmd = his.hist[++his_pos];
                        cout << cmd;
                        fflush(stdout);
                        continue;
                    }
                }
                else if (key == 68 || key == 67)
                {
                    //左右不想做了

                    continue;
                }
                else
                {
                    // 直接报错
                    exit(255);
                }
            }
        }

        // cout << "|" << cmd << "|" << endl;
        std::vector<std::string> args_p = split(cmd, "|"); //处理管道

        if (!args_p.empty())
        {
            his.hist.push_back(cmd);
        }
        // 没有可处理的命令
        if (args_p.empty())
        {
            continue;
        }
        //分离出了管道命令
        else if (args_p.size() != 1)
        {
            int cmd_count_p = args_p.size();
            int read_fd; //上一个管道的读端口
            for (int i = 0; i < cmd_count_p; i++)
            {
                int pipefd[2]; //创建了n次 但只用其中的n-1次
                int ret;
                if (i < cmd_count_p - 1)
                {
                    ret = pipe(pipefd);
                    if (ret < 0)
                    {
                        cout << "fail to create pipe" << endl;
                        continue;
                    }
                }
                else
                    ret = 1; //不使用的一次
                int pid = fork();
                //子进程
                if (pid == 0)
                {
                    //除了最后一次命令 否则都将stdout重定向至当前管道入口
                    if (ret != 1)
                    {
                        dup2(pipefd[WRITE_END], STDOUT_FILENO);
                    }
                    // close(pipefd[READ_END]);
                    //除了第一条指令 都将标准输入重定向到上一个管道出口
                    if (i)
                    {
                        dup2(read_fd, STDIN_FILENO);
                    }
                    cmd_out_p(args_p[i]);
                }
                if (i)
                    close(read_fd);
                if (i < cmd_count_p - 1)
                {
                    read_fd = pipefd[READ_END];
                }
                close(pipefd[WRITE_END]);
            }
            while (wait(nullptr) > 0)
                ;
        }
        //单条命令
        else
        {
            cmd_builtin(cmd);
        }
        cmd.clear();
    }
}

//执行单个命令
int cmd_builtin(string cmd)
{
    // 按空格分割命令为单词
    std::vector<std::string> args = split(cmd, " ");

    // 更改工作目录为目标目录
    if (args[0] == "cd")
    {
        if (args.size() <= 1)
        {
            // 输出的信息尽量为英文，非英文输出（其实是非 ASCII 输出）在没有特别配置的情况下（特别是 Windows 下）会乱码
            // 如感兴趣可以自行搜索 GBK Unicode UTF-8 Codepage UTF-16 等进行学习
            std::cout << "Insufficient arguments\n";
            // 不要用 std::endl，std::endl = "\n" + fflush(stdout)
            return 0;
        }

        // 调用系统 API
        int ret = chdir(args[1].c_str());
        if (ret < 0)
        {
            std::cout << "cd failed\n";
        }
        return 0;
    }

    // 显示当前工作目录
    if (args[0] == "pwd")
    {
        // 打印提示符
        std::string cwd_default; //用于提示符
        cwd_default.resize(PATH_MAX);
        const char *ret_default = getcwd(&cwd_default[0], PATH_MAX);
        if (ret_default == nullptr)
        {
            std::cout << "default cwd failed\n";
        }
        else
        {
            std::cout << ret_default << endl;
        }
        return 0;
    }

    // 设置环境变量
    if (args[0] == "export")
    {
        for (auto i = ++args.begin(); i != args.end(); i++)
        {
            std::string key = *i;

            // std::string 默认为空
            std::string value;

            // std::string::npos = std::string end
            // std::string 不是 nullptr 结尾的，但确实会有一个结尾字符 npos
            size_t pos;
            if ((pos = i->find('=')) != std::string::npos)
            {
                key = i->substr(0, pos);
                value = i->substr(pos + 1);
            }

            int ret = setenv(key.c_str(), value.c_str(), 1);
            if (ret < 0)
            {
                std::cout << "export failed\n";
            }
        }
        return 0;
    }
    if (args[0] == "history")
    {
        if (args.size() == 1)
        {
            if ((his.get_origin_size() + 1 != his.hist.size()) && his.hist[his.hist.size() - 2] == "history")
            {
                //加1因为新加了一条指令
                //连续两条history
                his.hist.pop_back();
                return 0;
            }
            //只有一个查询命令
            his.show();
            return 0;
        }
        his.show(stoi(args[1]));
        return 0;
    }
    if (args[0][0] == '!')
    {
        //! n和!！
        his.hist.pop_back();
        if (args.size() != 1)
        {
            cout << "无法识别该!指令" << endl;
            return 0;
        }
        int num;
        if (args[0][1] == '!')
        {
            //!!
            num = his.hist.size() - 1;
        }
        else
        {
            //! n
            num = stoi(args[0].substr(1)) - 1;
            if (num > his.hist.size() - 1)
            {
                cout << "不支持的history num:" << num + 1 << endl;
                return -1;
            }
        }
        cmd_builtin(his.hist[num]);
        return 0;
    }
    // 退出
    if (args[0] == "exit")
    {
        if (args.size() <= 1)
        {
            his.write();
            exit(0);
        }

        // std::string 转 int
        std::stringstream code_stream(args[1]);
        int code = 0;
        code_stream >> code;

        // 转换失败
        if (!code_stream.eof() || code_stream.fail())
        {
            std::cout << "Invalid exit code\n";
            return 0;
        }
        his.write();
        exit(code);
    }
    // 外部命令
    pid_t pid = fork();
    if (pid == 0)
    {
        // 这里只有子进程才会进入
        cmd_out_p(cmd);
    }
    int ret = wait(nullptr);
    if (ret < 0)
    {
        std::cout << "wait failed";
    }
    return 0;
}
//管道专用 执行一道外部命令
int cmd_out_p(string cmd)
{
    int fd[2];
    //对于管道也能运行 因为管道中stdin和stdout已经变成了pipefd的副本
    fd[READ_END] = STDIN_FILENO;
    fd[WRITE_END] = STDOUT_FILENO;
    // 按空格分割命令为单词
    std::vector<std::string> args = split(cmd, " ");
    cmd_redirect(args, fd);
    //没有重定向则相当于什么都没发生 重定向只可能出现在管道首命令和末命令
    dup2(fd[READ_END], STDIN_FILENO);
    dup2(fd[WRITE_END], STDOUT_FILENO);
    // std::vector<std::string> 转 char **
    char *arg_ptrs[args.size() + 1];
    for (auto i = 0; i < args.size(); i++)
    {
        arg_ptrs[i] = &args[i][0];
    }
    // exec p 系列的 argv 需要以 nullptr 结尾
    arg_ptrs[args.size()] = nullptr;
    execvp(args[0].c_str(), arg_ptrs);
    exit(255); //理论上不会到这里
}

//实现文件重定向
//重定向后删除该符号和文件名 所以使用了引用
int cmd_redirect(std::vector<string> &args, int *fd)
{
    for (vector<string>::iterator it = args.begin(); it != args.end();)
    {
        //表示是否成功打开文件
        int fsignal;
        //考虑到命令长度 逐条遍历效率比3次find效率更高
        if (*it == ">")
        {
            if ((it + 1) == args.end())
            {
                //没输入文件名
                cout << "Please enter file name" << endl;
                exit(Code_NoFileName);
            }
            fsignal = open((*(it + 1)).c_str(), O_WRONLY | O_CREAT, S_IRWXU);
            if (fsignal < 0)
            {
                cout << "fail when open" << *(it + 1) << ":" << strerror(errno) << endl;
            }
            else
            {
                // 重定向
                fd[WRITE_END] = fsignal;
            }
            //擦除重定向符号和文件名
            it = args.erase(it);
            it = args.erase(it);
        }
        else if (*it == ">>")
        {
            if ((it + 1) == args.end())
            {
                //没输入文件名
                cout << "Please enter file name" << endl;
                exit(Code_NoFileName);
            }
            fsignal = open((*(it + 1)).c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);
            if (fsignal < 0)
            {
                cout << "fail when open" << *(it + 1) << ":" << strerror(errno) << endl;
            }
            else
            {
                // 重定向
                fd[WRITE_END] = fsignal;
            }
            //擦除重定向符号和文件名
            it = args.erase(it);
            it = args.erase(it);
        }
        else if (*it == "<")
        {
            if ((it + 1) == args.end())
            {
                //没输入文件名
                cout << "Please enter file name" << endl;
                exit(Code_NoFileName);
            }
            fsignal = open((*(it + 1)).c_str(), O_RDONLY, S_IRWXU);
            if (fsignal < 0)
            {
                cout << "fail when open" << *(it + 1) << ":" << strerror(errno) << endl;
            }
            else
            {
                // 重定向
                fd[READ_END] = fsignal;
            }
            //擦除重定向符号和文件名
            it = args.erase(it);
            it = args.erase(it);
        }
        else
        {
            it++;
        }
    }
    return 0;
}

void signal_callback(int signum)
{
    pid_t pid = waitpid(0, NULL, WNOHANG);
    if (pid == -1)
    {
        cout << endl;
        siglongjmp(buf, 1);
    }
    else
    {
        cout << endl;
    }
}
