#pragma once
#include <string>
#include <map>

#define DEMO_GLOBAL_APP_KEY			"45c6af3c98409b18a84451215d0bdd6e"
#define DEMO_GLOBAL_TEST_APP_KEY	"fe416640c8e8a72734219e1847ad2547"

class QPath
{
public:
	static std::wstring GetAppPath(); //获取exe所在目录，最后有"\\"
	static std::wstring GetNimAppDataDir(const std::wstring& app_data_dir);  // "...Local\\Netease\\Nim\\"
	static void			AddNewEnvironment(const std::wstring& directory); // 添加一个路径到 exe 的环境变量
};

class QCommand
{
public:
	static void ParseCommand(const std::wstring &cmd);
	static bool AppStartWidthCommand(const std::wstring &app, const std::wstring &cmd);
	static bool RestartApp(const std::wstring &cmd);
	static std::wstring Get(const std::wstring &key);
	static void Set(const std::wstring &key, const std::wstring &value);
	static void Erase(const std::wstring &key);
private:
	static std::map<std::wstring,std::wstring> key_value_;
};