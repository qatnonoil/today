#include "pch.hpp"

std::string g_txtPath;

/*
-----------------------------------------------
日付回り
-----------------------------------------------
*/
struct Date
{
public:
    // 今日からの日付の差分指定で作成する。
    // 負の数を指定すれば前の日になる。
    Date(int32_t dayDif = 0)
    {
        time_t rawTime;
        time(&rawTime);
        t_ = localtime(&rawTime);
        t_->tm_mday += dayDif;
        mktime(t_);
    }
    int32_t year() const
    {
        return t_->tm_year + 1900;
    }
    int32_t month() const
    {
        return t_->tm_mon + 1;
    }
    int32_t mday() const
    {
        return t_->tm_mday;
    }
    std::string toString() const
    {
        std::stringstream ss;
        ss << year() << "_" << month() << "_" << mday();
        return ss.str();
    }
    std::string createFileName() const
    {
        std::string fileName = g_txtPath + toString() + ".txt";
        return fileName;
    }
private:
    tm* t_;
};

/*
-----------------------------------------------
-----------------------------------------------
*/
struct FileStat
{
    bool exist = false;
    int32_t fileSize = 0;
};

/*
-----------------------------------------------
ファイルが存在するか
-----------------------------------------------
*/
FileStat fileStat(const std::string& fileName)
{
    //
    struct _stat s;
    const bool fileExist = (_stat(fileName.c_str(), &s) == 0);
    FileStat fs;
    fs.exist = fileExist;
    fs.fileSize = s.st_size;
    return fs;
}

/*
-----------------------------------------------
-----------------------------------------------
*/
void openFile(const std::string& fileName)
{
    if (!fileStat(fileName).exist)
    {
        // ファイルを生成する
        const std::string command = std::string("type nul > ") + fileName;
        system(command.c_str());
    }
    // ファイルを開く
    const std::string command = "START " + fileName;
    system(command.c_str());
}

/*
-----------------------------------------------
-----------------------------------------------
*/
class Command
{
public:
    virtual std::string name() = 0;
    virtual std::string description() = 0;
    virtual void exec(int32_t argc, char* argv[]) = 0;
};
std::vector<std::shared_ptr<Command>> g_commands;

/*
-----------------------------------------------
今日のファイルを開く
-----------------------------------------------
*/
class CommandToday
    :public Command
{
public:
    CommandToday(){}
    virtual std::string name() override
    {
        return "today";
    }
    virtual std::string description() override
    {
        return "create/open today file.";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        // 今日の日付を得る
        Date date(0);
        const std::string fileName = date.createFileName();
        // ファイルを開く
        openFile(fileName);
    }
};

/*
-----------------------------------------------
指定した分の前のファイルを開く
-----------------------------------------------
*/
class CommandPrev
    :public Command
{
public:
    CommandPrev() {}
    virtual std::string name() override
    {
        return "prev";
    }
    virtual std::string description() override
    {
        return "create/open prev today file.";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        // 日付の差分を得る
        const int32_t dayDiffNum = (argc == 2) ? 1 : atoi(argv[2]);
        int32_t dayDiff = 0;
        const int32_t dayDiffLimit = -30;
        for (int32_t i = 0; i < dayDiffNum; ++i)
        {
            Date date;
            do
            {
                dayDiff -= 1;
                date = Date(dayDiff);
            } while (!fileStat(date.createFileName()).exist && dayDiff > dayDiffLimit);
            if (dayDiff == dayDiffLimit)
            {
                break;
            }
            // ファイルを開く
            openFile(date.createFileName());
        }
    }
};

/*
-----------------------------------------------
空のファイルを削除する
-----------------------------------------------
*/
class CommandGC
    :public Command
{
public:
    CommandGC() {}
    virtual std::string name() override
    {
        return "gc";
    }
    virtual std::string description() override
    {
        return "delete empty today file.";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        // 日付の差分を得る
        int32_t dayDiff = 0;
        // HACK: とりあえず三か月前まで。
        const int32_t dayDiffLimit = -90;
        for (int32_t i = 0; i > dayDiffLimit; --i)
        {
            Date date(i);
            const FileStat fs = fileStat(date.createFileName());
            // ファイルが存在し、サイズが0kbであるならば削除
            if (fs.exist && fs.fileSize == 0)
            {
                printf("delete %s\n", date.createFileName().c_str());
                std::string command = "del " + date.createFileName();
                system(command.c_str());
            }
        }
    }
};

/*
-----------------------------------------------
ヘルプを表示する
-----------------------------------------------
*/
class CommandHelp
    :public Command
{
public:
    CommandHelp() {}
    virtual std::string name() override
    {
        return "help";
    }
    virtual std::string description() override
    {
        return "show help.";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        for (const auto& cmd : g_commands)
        {
            printf("%s : %s\n", cmd->name().c_str(), cmd->description().c_str());
        }
    }
};

/*
-----------------------------------------------
バージョンを表示する
-----------------------------------------------
*/
class CommandVersion
    :public Command
{
public:
    CommandVersion() {}
    virtual std::string name() override
    {
        return "version";
    }
    virtual std::string description() override
    {
        return "show version.";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        printf("Build time: %s %s", __DATE__, __TIME__);
    }
};

/*
-----------------------------------------------
-----------------------------------------------
*/
void main(int32_t argc, char* argv[])
{
    // カレントディレクトリをexeがあるパスにする
    char binFilePath[MAX_PATH];
    ::GetModuleFileNameA(nullptr, binFilePath, sizeof(binFilePath) / sizeof(*binFilePath));
    ::PathRemoveFileSpec(binFilePath);
    SetCurrentDirectory(binFilePath);

    // テキストのパス
    char txtPath[MAX_PATH];
    GetPrivateProfileString("config", "txtdir", "", txtPath, sizeof(txtPath) / sizeof(*txtPath), "./today.ini");
    g_txtPath = txtPath;
    printf("txt path [%s]\n", txtPath);
    // コマンドが指定されていなければ今日のファイルを作成する
    if (argc == 1)
    {
        CommandToday ct;
        ct.exec(argc, argv);
        return;
    }
    // 全てのコマンドを生成する
    g_commands.clear();
    g_commands.emplace_back(std::make_shared<CommandToday>());
    g_commands.emplace_back(std::make_shared<CommandPrev>());
    g_commands.emplace_back(std::make_shared<CommandGC>());
    g_commands.emplace_back(std::make_shared<CommandHelp>());
    g_commands.emplace_back(std::make_shared<CommandVersion>());
    const std::string cmd = std::string(argv[1]);
    //
    for (const auto& command : g_commands)
    {
        if (command->name() == cmd)
        {
            printf("%s\n", command->description().c_str());
            command->exec(argc, argv);
            return;
        }
    }
    printf("invalid command.\n");
}
