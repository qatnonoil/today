// C
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <ctime>

// C++
#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include <iostream>
#include <iomanip>
#include <fstream>

//
#include <windows.h>
#include <Shlwapi.h>
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"Shlwapi.lib")


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
        const time_t rawTime = time(nullptr);
        localtime_s(&t_, &rawTime);
        //
        t_.tm_mday += dayDif;
        mktime(&t_);
    }
    // 日付直接指定
    Date(int32_t month, int32_t mday)
    {
        const time_t rawTime = time(nullptr);
        localtime_s(&t_, &rawTime);
        // 今年のその日が終わっていたら来年扱い
        if ((month < t_.tm_mon) || (t_.tm_mon == month && mday < t_.tm_mday ))
        {
            ++t_.tm_year;
        }
        //
        t_.tm_mon = month-1;
        t_.tm_mday = mday;
    }
    int32_t year() const
    {
        return t_.tm_year + 1900;
    }
    int32_t month() const
    {
        return t_.tm_mon + 1;
    }
    int32_t mday() const
    {
        return t_.tm_mday;
    }
    int32_t weekDay() const
    {
        return t_.tm_wday;
    }
    std::string toStringYYYYMMDD() const
    {
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << year() << "_" << std::setw(2) << std::setfill('0') << month() << "_" << std::setw(2) << std::setfill('0') << mday();
        return ss.str();
    }
    std::string toStringYYYYMM() const
    {
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << year() << "_" << std::setw(2) << std::setfill('0') << month();
        return ss.str();
    }
    std::string toStringYYYY() const
    {
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << year();
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
    tm t_ = {};
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
static void openFile(const std::string& fileName, bool forceCreate, const std::string& appendStr = "")
{
    //
    if (!fileStat(fileName).exist)
    {
        // 存在しなければ開かなくていい場合はこのまま終了
        if (!forceCreate)
        {
            return;
        }
        std::ofstream file;
        file.open(fileName);
        file << appendStr;
    }
    // ファイルを開く
    const std::string command = "START " + fileName;
    system(command.c_str());
}

/*
-----------------------------------------------
月の指定があるファイルを開く
-----------------------------------------------
*/
static void openMonthFile(const std::string& fileName)
{
    Date date;
    std::string fileNameFull = g_txtPath + "shelf/" +date.toStringYYYYMM() + std::string("_") + fileName + ".txt";
    openFile(fileNameFull, true);
}

/*
-----------------------------------------------
年の指定があるファイルを開く
-----------------------------------------------
*/
static void openYearFile(const std::string& fileName )
{
    Date date;
    std::string fileNameFull = g_txtPath + "shelf/" + date.toStringYYYY() + std::string("_") + fileName + ".txt";
    openFile(fileNameFull, true);
}

/*
-----------------------------------------------
一意なファイル名を持つファイルを開く
-----------------------------------------------
*/
static void openUniqueFile(const std::string& fileName)
{
    std::string fileNameFull = g_txtPath + "shelf/" + fileName + ".txt";
    openFile(fileNameFull, true);
}

/*
-----------------------------------------------
指定したファイルの内容全てをロードして文字列として返す
-----------------------------------------------
*/
static std::string readFileAll(const std::string& fileName)
{
    std::ifstream file(fileName.c_str());;
    std::istreambuf_iterator<char> it(file);
    std::istreambuf_iterator<char> last;
    std::string str(it, last);
    return str;
}

/*
-----------------------------------------------
指定した日のtemplateファイルのテキストを返す
-----------------------------------------------
*/
static std::string templateString(const Date& date)
{
    // 毎日テキスト
    const std::string everydayFileName = std::string("template/毎日.md");
    const std::string everydayTxt = readFileAll(g_txtPath + everydayFileName);

    // 曜日テキスト
    const std::array<const char*, 7> wdayStr =
    {
        "日曜",
        "月曜",
        "火曜",
        "水曜",
        "木曜",
        "金曜",
        "土曜"
    };
    const std::string wdayFileName = std::string("template/") + wdayStr[date.weekDay()] + ".md";
    const std::string wdayTxt = readFileAll(g_txtPath + wdayFileName);

    // 日付テキスト
    const std::string mdayFileName = std::string("template/") + std::to_string(date.mday()) + "日.md";
    const std::string mdayTxt = readFileAll(g_txtPath + mdayFileName);

    // 全てのテキストを返す
    return everydayTxt + wdayTxt + mdayTxt;
}

/*
-----------------------------------------------
指定したフォルダ以下の全てのファイルを列挙する
-----------------------------------------------
*/
static std::vector<std::string> getFileList(const std::string& folderPath)
{
    HANDLE hFind;
    WIN32_FIND_DATA fd;
    const std::string folderPathWithWild = folderPath + "*.*";
    hFind = FindFirstFile(folderPathWithWild.c_str(), &fd);
    std::vector<std::string> fileList;

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return fileList;
    }
    //
    do
    {
        const bool isFolder = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        // フォルダーの場合
        if (isFolder)
        {
            const bool invalidPath =
                (strcmp(fd.cFileName, ".") == 0) ||
                (strcmp(fd.cFileName, "..") == 0);
            if (!invalidPath)
            {
                const std::string subFolderPath = folderPath + std::string(fd.cFileName) + "\\";
                auto subFolderList = getFileList(subFolderPath);
                fileList.insert(fileList.end(), subFolderList.begin(), subFolderList.end());
            }
        }
        // ファイルの場合
        else
        {
            fileList.push_back(folderPath + fd.cFileName);
        }
    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);

    return fileList;
}

/*
-----------------------------------------------
全てのテキストのリストを返す
-----------------------------------------------
*/
static std::vector<std::string> getAllTextFileList()
{
    return getFileList(g_txtPath);
}

/*
-----------------------------------------------
指定した日のファイルを開く
-----------------------------------------------
*/
static void openDateFile(const Date& date, bool forceCreate)
{
    //
    const std::string fileName = date.createFileName();
    // ファイルを開く
    openFile(fileName, forceCreate, templateString(date));
}

/*
-----------------------------------------------
system()代替。起動後すぐ戻る。
-----------------------------------------------
*/
static void doCommand(const std::string& command)
{
    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    CreateProcess(
        nullptr, const_cast<char*>(command.c_str()),
        nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS,
        nullptr, nullptr, &si, &pi);
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
        // 今日のdateファイルを開く
        openDateFile(Date(0), true);
    }
};

/*
-----------------------------------------------
全てのファイルを開く(実際は前後30日のファイルを開く)
-----------------------------------------------
*/
class CommandAll
    :public Command
{
public:
    CommandAll() {}
    virtual std::string name() override
    {
        return "all";
    }
    virtual std::string description() override
    {
        return "open all today file.";
    }
    virtual void exec(int32_t argc, char* argv[]) override
    {
        // 前後30日のファイルを全て開く
        for (int32_t i=-30;i<=30;++i)
        {
            openDateFile(Date(i), false);
        }
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
            openDateFile(date, false);
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
        today goto 01/01
        today goto 1/1
        today goto 12/31
        */
        int32_t month, mday;
        if (sscanf_s(argv[2], "%d/%d", &month, &mday) == 2)
        {
            openDateFile(Date(month, mday), true);
            return;
        }
        /*
        today goto +3
        today goto -3
        */
        int32_t dayDiff = 0;
        if (sscanf_s(argv[2], "%d", &dayDiff) == 1)
        {
            // ファイルを開く
            openDateFile(Date(dayDiff), true);
            return;
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
        //sakura.exe -GREPMODE -GFOLDER="C:\Users\admin\Dropbox\today" -GKEY="Keyword" -GOPT=P -TYPE=txt,md
        if (argc <= 2)
        {
            printf("no grep keyword.");
            return;
        }
        // 検索文字列
        const char* key = argv[2];
        // sakuraの起動
        const std::string command =
            "\"" + g_sakuraPath + "\" -GREPMODE -GFOLDER=" + g_txtPath +
            " -GKEY=\"" + std::string(key) + "\"  -GOPT=PS -GCODE=99 -GFILE=*.txt,*.md";
        printf("cmd %s\n", command.c_str());
        doCommand(command);
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
        // todayフォルダを開く
        const std::string command = "explorer " + g_txtPath;
        doCommand(command.c_str());
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
std::string configString(
    const std::string& region,
    const std::string& config,
    const std::string& defaultValue)
{
    const char* iniFile = "./today.ini";
    char retValue[MAX_PATH];
    GetPrivateProfileString(region.c_str(), config.c_str(), defaultValue.c_str(), retValue, sizeof(retValue) / sizeof(*retValue), iniFile);
    std::string ret;
    ret.assign(retValue);
    return ret;
}

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
    g_txtPath = configString("config", "txtdir", "");
    printf("txt path [%s]\n", g_txtPath.c_str());

    // sakuraのパス
    g_sakuraPath = configString("config", "sakuraPath", "");
    printf("sakura path [%s]\n", g_sakuraPath.c_str());

    // 指定テキストフォルダ以下で空のファイルは全て削除する
    for (auto& path : getAllTextFileList())
    {
        const FileStat stat = fileStat(path);
        if (stat.exist && 
            stat.fileSize == 0)
        {
            DeleteFile(path.c_str());
        }
    }

    {
        // 今日のバックアップが存在しなければバックアップを作成する
        const std::string todayBackupFileName = g_txtPath + "backup\\" + Date(0).toStringYYYYMMDD() + ".zip";
        const FileStat stat = fileStat(todayBackupFileName);
        if (!stat.exist)
        {
            // "7za a [zipファイルパス] [zip元フォルダ] -xr!backup"
            const std::string command = "7za a " + todayBackupFileName + " " + g_txtPath + " -xr!backup";
            system(command.c_str());
        }
    }

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
    g_commands.emplace_back(std::make_shared<CommandAll>());
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

    // プリセットのコマンドになければカスタムされたコマンド
    const std::string customCmd = configString("custom", cmd, "");
    if (customCmd != "")
    {
        if (customCmd == "month")
        {
            openMonthFile(cmd);
        }
        else if (customCmd == "year")
        {
            openYearFile(cmd);
        }
        else if (customCmd == "unique")
        {
            openUniqueFile(cmd);
        }
        else
        {
            printf("invalid custom command.\n");
        }
        return;
    }

    //
    printf("invalid command.\n");
}
