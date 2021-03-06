﻿#if defined(_MSC_VER)
#define WINDOWS
#else
#define CLANG
#endif

//
#define _CRT_SECURE_NO_WARNINGS

// C
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <cassert>

// C++
#include <array>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <memory>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>

//
#if defined(WINDOWS)
#include <windows.h>
#endif

#if defined(CLANG)
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <mach-o/dyld.h>
#endif

std::string g_txtPath;
std::string g_sakuraPath;

/*
 -----------------------------------------------
 system()代替。起動後すぐ戻る。
 -----------------------------------------------
 */
static void doCommand(const std::string& command)
{
#if defined(WINDOWS) && false
    PROCESS_INFORMATION pi = { 0 };
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    CreateProcess(
                  nullptr, const_cast<char*>(command.c_str()),
                  nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS,
                  nullptr, nullptr, &si, &pi);
#else
    system(command.c_str());
#endif
}

/*
-----------------------------------------------
日付回り
-----------------------------------------------
*/
static int32_t numDay(int32_t year, int32_t month, int32_t day)
{
    // 現在時刻を得る
    const time_t now = time(nullptr);

    // 目的の日時の設定
    tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    const time_t aim = mktime(&t);

    // 差の計算
    const long diffTimeInMs = long(difftime(aim, now));
    return int32_t(diffTimeInMs / (60 * 24 * 60)) + 1;
}

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
        t_ = *localtime(&rawTime);
        //
        t_.tm_mday += dayDif;
        mktime(&t_);
    }
    // 日付直接指定
    Date(int32_t month, int32_t mday)
    {
        const time_t rawTime = time(nullptr);
        t_ = *localtime(&rawTime);
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
    // ファイルが存在するか
    bool exist = false;
    // ファイルが空か。BOM付の場合はBOMを除いたテキスト部分のファイルサイズが0か。
    bool empty = false;
};

/*
 -----------------------------------------------
 ファイルが存在するか
 -----------------------------------------------
 */
static FileStat fileStat(const std::string& fileName)
{
    bool exist = false;
    bool empty = false;
    FILE* file = nullptr;
    file = fopen(fileName.c_str(), "rb");
    if (file != nullptr)
    {
        exist = true;
        //
        const int32_t firstCh = fgetc(file);
        if (firstCh == EOF)
        {
            empty = true;
        }
        // BOM
        else if(
            (0xEF == firstCh) &&
            (0xBB == fgetc(file)) &&
            (0xBF == fgetc(file)) )
        {
            // BOMのみだったら空ファイル扱い
            if (fgetc(file) == EOF)
            {
                empty = true;
            }
        }
        fclose(file);
    }

    FileStat fs;
    fs.exist = exist;
    fs.empty = empty;
    return fs;
}

/*
 -----------------------------------------------
 IniFile
 -----------------------------------------------
 */
class IniFile
{
public:
    IniFile() = default;
    IniFile(const std::string& fileName)
    {
        //
        printf("IniFile: %s\n", fileName.c_str());
        // ファイルを開く
        FILE* file = fopen(fileName.c_str(), "rt");
        if (file == nullptr)
        {
            printf("Cant open inifile\n");
            return;
        }
        //
        typedef Regions::iterator RegsIte;
        RegsIte regsIte;
        regions.insert({ std::string(""), Configs() });
        regsIte = regions.find(std::string(""));
        // 全ての行を読み取っていく
        char buffer[0xff];
        while (fgets(buffer, 0xff, file) != nullptr)
        {
            // Regionの開始
            char regionName[0xff];
            char key[0xff];
            char value[0xff];
            if (sscanf(buffer, "[%[^]]", regionName) == 1)
            {
                regions.insert({ std::string(regionName), Configs() });
                regsIte = regions.find(std::string(regionName));
            }
            // 「A="B"」の形式
            else if (sscanf(buffer, "%[^=]=\"%[^\"]\"", key, value) == 2)
            {
                Configs& configs = regsIte->second;
                configs.insert(std::make_pair<>(std::string(key), std::string(value)));
            }
            else if(strcmp(buffer,"\n") == 0 )
            {
                // 何もしない
            }
            else
            {
                printf("IniFile parse error.\n");
            }
        }
        //
        fclose(file);
    }
    std::string get(const std::string& region, const std::string& key, const std::string& defalutValue) const
    {
        //
        auto regionIte = regions.find(region);
        if (regionIte == regions.end())
        {
            return defalutValue;
        }
        //
        const Configs& configs = regionIte->second;
        auto configIte = configs.find(key);
        if (configIte == configs.end())
        {
            return defalutValue;
        }
        else
        {
            return configIte->second;
        }
    }
private:
    typedef std::unordered_map<std::string, std::string> Configs;
    typedef std::unordered_map<std::string, Configs> Regions;
    Regions regions;
};
//
static IniFile g_iniFile;


/*
 -----------------------------------------------
 -----------------------------------------------
 */
static void openFile(const std::string& fileName,
                     bool forceCreate,
                     const std::string& appendStr = "")
{
    //
    if (!fileStat(fileName).exist)
    {
        // 存在しなければ開かなくていい場合はこのまま終了
        if (!forceCreate)
        {
            return;
        }
#if defined(CLANG)
        これ必要？
        const std::string command = "touch " + fileName;
        doCommand(command.c_str());
#endif
        // BOMを付け、追加文字も書き込む
        std::ofstream file(fileName);
        unsigned char bom[] = { 0xEF,0xBB,0xBF };
        file.write((char*)bom, sizeof(bom));
        file << appendStr;
        file.close();
    }
    // ファイルを開く
#if defined(WINDOWS)
    const std::string command = "START " + fileName;
#else
    const std::string command = "open " + fileName;
#endif
    doCommand(command.c_str());
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

    // 日を追加
    const int32_t year = std::atoi(g_iniFile.get("goal", "year", "").c_str());
    const int32_t month = std::atoi(g_iniFile.get("goal", "month", "").c_str());
    const int32_t day = std::atoi(g_iniFile.get("goal", "day", "").c_str());
    const std::string rday = "remain: " + std::to_string(numDay(year, month, day)) + "day.";
    
    // 全てのテキストを返す
    return everydayTxt + wdayTxt + mdayTxt + rday;
}

/*
-----------------------------------------------
全てのテキストのリストを返す
-----------------------------------------------
*/
static std::vector<std::string> getFileList(const std::string& folderPath)
{
    std::vector<std::string> ret;
    for (auto& f : std::filesystem::recursive_directory_iterator(folderPath))
    {
        if (f.is_directory())
        {
            continue;
        }
        ret.push_back(f.path().string());
    }
    return ret;
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
        if (sscanf(argv[2], "%d/%d", &month, &mday) == 2)
        {
            openDateFile(Date(month, mday), true);
            return;
        }
        /*
         today goto +3
         today goto -3
         */
        int32_t dayDiff = 0;
        if (sscanf(argv[2], "%d", &dayDiff) == 1)
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
        
#if defined(WINDOWS)
        // sakuraの起動
        const std::string command =
        "\"\"" + g_sakuraPath + "\" -GREPMODE -GFOLDER=" + g_txtPath +
        " -GKEY=\"" + std::string(key) + "\"  -GOPT=PS -GCODE=99 -GFILE=*.txt,*.md\"";
        printf("cmd %s\n", command.c_str());
        doCommand(command);
#else
        // TODO:
        const std::string cmd = "grep -r " + std::string(key) + " " + g_txtPath;
        printf("%s\n", cmd.c_str());
        doCommand("pwd");
        doCommand(cmd.c_str());
#endif
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
#if defined(WINDOWS)
        const std::string command = "explorer " + g_txtPath;
#else
        const std::string command = std::string("open ") + g_txtPath;
#endif
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
 実行ファイルのあるディレクトリを返す
 -----------------------------------------------
 */
static std::string getExeDir()
{
    std::string aux;
#if defined(_WIN32) || defined(WIN32)
    // aliasから起動した場合、argv[0]にフルパスは入っていないので明示的に取り出す
    char fullPath[MAX_PATH + 1];
    GetModuleFileName(nullptr, fullPath, MAX_PATH);
    aux.assign(fullPath);
    size_t pos = aux.rfind('\\');
#else
    char fullPath[1024];
    uint32_t size = sizeof(fullPath);
    _NSGetExecutablePath(fullPath, &size);
    aux.assign(fullPath);
    size_t pos = aux.rfind('/');
#endif
    return aux.substr(0,pos+1);
}

/*
 -----------------------------------------------
 -----------------------------------------------
 */
int32_t main(int32_t argc, char* argv[])
{
    // 
    printf("Build(%s)\n", __DATE__);
    // 実行ファイルのパスを取得
    const std::string exePath = getExeDir();
    printf("Exe: %s\n", exePath.c_str());
    // IniFileを開く
    g_iniFile = IniFile(exePath + "today.ini");
    // テキストのパス
    g_txtPath = exePath;
    // sakuraのパス
    g_sakuraPath = g_iniFile.get("config", "sakuraPath", "");
    printf("sakura path %s\n", g_sakuraPath.c_str());
    
    // 指定テキストフォルダ以下で空のファイルは全て削除する
    for (auto& path : getAllTextFileList())
    {
        const FileStat stat = fileStat(path);
        if (stat.exist && stat.empty )
        {
            printf("Delete [%s]\n", path.c_str());
            std::remove(path.c_str());
        }
    }
    
    {
        // 今日のバックアップが存在しなければバックアップを作成する
        const std::string todayBackupFileName = g_txtPath + "backup/" + Date(0).toStringYYYYMMDD() + ".zip";
        const FileStat stat = fileStat(todayBackupFileName);
        if (!stat.exist)
        {
            // "7za a [zipファイルパス] [zip元フォルダ] -xr!backup"
            const std::string command = "7za a " + todayBackupFileName + " " + g_txtPath + " -xr!backup";
            doCommand(command.c_str());
        }
    }
    
    // コマンドが指定されていなければ今日のファイルを作成する
    if (argc == 1)
    {
        CommandToday ct;
        ct.exec(argc, argv);
        return 0;
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
            return 0;
        }
    }
    
    // プリセットのコマンドになければカスタムされたコマンド
    const std::string customCmd = g_iniFile.get("custom", cmd, "");
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
        return 0;
    }
    
    //
    printf("invalid command.\n");
    //
    
    return 0;
}
