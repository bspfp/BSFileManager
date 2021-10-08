#include "stdafx.h"
#include "CommandLineParser.h"

bool CommandLineParser::parse()
{
	std::wstring tmp_exeFilePath;
	std::map<std::wstring, std::wstring> tmp_options;
	std::vector<std::wstring> tmp_args;

	int argc = 0;
	wchar_t** args = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (args == nullptr)
		return false;
	tmp_exeFilePath = args[0];
	std::wstring opt;
	for (int i = 1; i < argc; ++i)
	{
		if (args[i][0] == L'/')
		{
			opt = args[i] + 1;
			if (opt.length() > 0)
				tmp_options[opt].clear();
			else
				return false;
		}
		else
		{
			if (opt.length() > 0)
			{
				tmp_options[opt] = args[i];
				opt.clear();
			}
			else
			{
				tmp_args.push_back(args[i]);
			}
		}
	}
	_exeFilePath.swap(tmp_exeFilePath);
	_options.swap(tmp_options);
	_args.swap(tmp_args);

	{
		wchar_t drive[_MAX_DRIVE] = L"";
		wchar_t dir[_MAX_DIR] = L"";
		_wsplitpath_s(_exeFilePath.c_str(), drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
		_exeFileFolder = drive;
		_exeFileFolder += dir;
	}

	return true;
}

std::wstring const & CommandLineParser::getExeFilePath() const
{
	return _exeFilePath;
}

std::wstring const & CommandLineParser::getExeFileFolder() const
{
	return _exeFileFolder;
}

std::wstring const & CommandLineParser::getParam(std::wstring const & opt, std::wstring const & defaultValue) const
{
	auto iter = _options.find(opt);
	if (iter != _options.end())
		return iter->second;
	return defaultValue;
}

bool CommandLineParser::hasOption(std::wstring const & opt) const
{
	auto iter = _options.find(opt);
	return (iter != _options.end());
}

std::vector<std::wstring> const & CommandLineParser::getArgs() const
{
	return _args;
}