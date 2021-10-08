#pragma once

#include <string>
#include <map>

class CommandLineParser
{
public:
	bool parse();

	std::wstring const & getExeFilePath() const;
	std::wstring const & getExeFileFolder() const;
	std::wstring const & getParam(std::wstring const & opt, std::wstring const & defaultValue = L"") const;
	bool hasOption(std::wstring const & opt) const;
	std::vector<std::wstring> const & getArgs() const;

private:
	std::wstring _exeFilePath;
	std::wstring _exeFileFolder;
	std::map<std::wstring, std::wstring> _options;
	std::vector<std::wstring> _args;
};
