#include "pch.hpp"

std::string g_txtPath;
std::string g_sakuraPath;

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
    std::string toStringYYYYMMDD() const
    {
        std::stringstream ss;
        ss << year() << "_" << month() << "_" << mday();
        return ss.str();
    }
    std::string toStringYYYYMM() const
    {
        std::stringstream ss;
        ss << year() << "_" << month();
        return ss.str();
    }
    std::string createFileName() const
    {
        std::string fileName = g_txtPath + toStringYYYYMMDD() + ".txt";
        return fileName;
    }
    std::string createFileNameYYYYMM() const
    {
        std::string fileName = g_txtPath + toStringYYYYMM() + ".txt";
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
static void openFile(const std::string& fileName)
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
static void openMonthFile(const std::string& fileName)
{
    Date date;
    std::string fileNameFull = g_txtPath + fileName + std::string("_") + date.toStringYYYYMM() + ".txt";
    openFile(fileNameFull);
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
goto
-----------------------------------------------
*/
class CommandGoto
    :public Command
{
public:
    CommandGoto() {}
    virtual std::string name() override
    {
        return "goto";
    }
    virtual std::string description() override
    {
        return "goto specified day and \"today\".";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        if (argc <= 2)
        {
            return;
        }
        /*
        today goto +3
        today goto -3
        */
        int32_t dayDiff = 0;
        if (sscanf(argv[2], "%d", &dayDiff) != 1)
        {
            return;
        }
        Date date(dayDiff);
        // ファイルを開く
        openFile(date.createFileName());
    }
};

/*
-----------------------------------------------
tmpファイルを作成する
-----------------------------------------------
*/
class CommandTmp
    :public Command
{
public:
    CommandTmp() {}
    virtual std::string name() override
    {
        return "tmp";
    }
    virtual std::string description() override
    {
        return "create tmp file.";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        // ファイル名を生成する
        const std::size_t hash = std::hash<int32_t>{}(timeGetTime());
        const std::string hasStr = g_txtPath + std::string("tmp_") + std::to_string(hash&0xFFFF) + std::string(".txt");
        // ファイルを開く
        openFile(hasStr);
    }
};

/*
-----------------------------------------------
todoファイルを作成する
-----------------------------------------------
*/
class CommandTodo
    :public Command
{
public:
    CommandTodo() {}
    virtual std::string name() override
    {
        return "todo";
    }
    virtual std::string description() override
    {
        return "open todo file.";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        openMonthFile("todo");
    }
};

/*
-----------------------------------------------
doneファイルを作成する
-----------------------------------------------
*/
class CommandDone
    :public Command
{
public:
    CommandDone() {}
    virtual std::string name() override
    {
        return "done";
    }
    virtual std::string description() override
    {
        return "open done file.";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        openMonthFile("done");
    }
};

/*
-----------------------------------------------
memoファイルを作成する
-----------------------------------------------
*/
class CommandMemo
    :public Command
{
public:
    CommandMemo() {}
    virtual std::string name() override
    {
        return "memo";
    }
    virtual std::string description() override
    {
        return "open memo file.";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        openMonthFile("memo");
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
grepの実行
-----------------------------------------------
*/
class CommandGrep
    :public Command
{
public:
    CommandGrep() {}
    virtual std::string name() override
    {
        return "grep";
    }
    virtual std::string description() override
    {
        return "grep files.";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        //sakura.exe -GREPMODE -GFOLDER="C:\Users\admin\Dropbox\today" - GKEY = "床" - GOPT = P
        if (argc <= 2)
        {
            printf("no grep keyword.");
            return;
        }
        // 検索文字列
        const char* key = argv[2];
        // sakuraの起動
        const std::string command =
            "\"\"" + g_sakuraPath + "\" -GREPMODE -GFOLDER=" + g_txtPath +
            " -GKEY=\"" + std::string(key) + "\"  -GOPT=PS -GCODE=99\"";
        printf("%s\n", command.c_str());
        system(command.c_str());
    }
};

/*
-----------------------------------------------
todayフォルダを開く
-----------------------------------------------
*/
class CommandOpen
    :public Command
{
public:
    CommandOpen() {}
    virtual std::string name() override
    {
        return "open";
    }
    virtual std::string description() override
    {
        return "oepn today folder.";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        // TODO: todayフォルダを開く
        const std::string command = "explorer " + g_txtPath;
        system(command.c_str());
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

    char sakuraPath[MAX_PATH];
    GetPrivateProfileString("config", "sakuraPath", "", sakuraPath, sizeof(sakuraPath) / sizeof(*sakuraPath), "./today.ini");
    g_sakuraPath = sakuraPath;
    printf("sakura path [%s]\n", g_sakuraPath.c_str());

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
    g_commands.emplace_back(std::make_shared<CommandGoto>());
    g_commands.emplace_back(std::make_shared<CommandTmp>());
    g_commands.emplace_back(std::make_shared<CommandTodo>());
    g_commands.emplace_back(std::make_shared<CommandDone>());
    g_commands.emplace_back(std::make_shared<CommandMemo>());
    g_commands.emplace_back(std::make_shared<CommandGC>());
    g_commands.emplace_back(std::make_shared<CommandGrep>());
    g_commands.emplace_back(std::make_shared<CommandOpen>());
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
