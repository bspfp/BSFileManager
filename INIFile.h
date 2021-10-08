#pragma once

#include <string>
#include <locale>
#include <map>
#include <sstream>

class INIFile
{
public:
	INIFile()
		: _loc(std::locale(std::locale::classic(), ".OCP", std::locale::ctype | std::locale::collate))
	{
	}

	bool load(std::wstring const & filepath)
	{
		_data.clear();
		FILE* fp = nullptr;
		if (_wfopen_s(&fp, filepath.c_str(), L"rt, ccs=UTF-16LE") != 0)
			return false;
		std::wstring group;
		std::wstring rbuf;
		wchar_t buf[4096];
		for (;;)
		{
			if (fgetws(buf, 4096, fp) == nullptr)
				break;
			rbuf = _trim(buf);
			rbuf = _removeComments(rbuf);
			if (rbuf.length() < 1)
				continue;
			_insertData(group, rbuf);
		}
		fclose(fp);
		return true;
	}

	bool save(std::wstring const & filepath)
	{
		FILE* fp = nullptr;
		if (_wfopen_s(&fp, filepath.c_str(), L"wt, ccs=UTF-16LE") != 0)
			return false;

		bool bFirstGroup = true;
		for (auto iterG = _data.begin(); iterG != _data.end(); ++iterG)
		{
			if (iterG->first.length() > 0 || !bFirstGroup)
			{
				fwprintf_s(fp, L"[%s]\n", iterG->first.c_str());
				bFirstGroup = false;
			}
			for (auto iterK = iterG->second.begin(); iterK != iterG->second.end(); ++iterK)
			{
				// multi value
				fwprintf_s(fp, L"%s=%s\n", iterK->first.c_str(), iterK->second.at(0).c_str());
			}
		}
		fclose(fp);
		return true;
	}

	const std::map<std::wstring, std::vector<std::wstring>>* get(std::wstring const & group) const
	{
		auto iterG = _data.find(group);
		if (iterG != _data.end())
		{
			return &(iterG->second);
		}
		return nullptr;
	}

	const std::vector<std::wstring>* get(std::wstring const & group, std::wstring const & key) const
	{
		auto iterG = _data.find(group);
		if (iterG != _data.end())
		{
			auto iterK = iterG->second.find(key);
			if (iterK != iterG->second.end())
			{
				return &iterK->second;
			}
		}
		return nullptr;
	}

	std::wstring get(std::wstring const & group, std::wstring const & key, std::wstring const & defVal) const
	{
		auto iterG = _data.find(group);
		if (iterG != _data.end())
		{
			auto iterK = iterG->second.find(key);
			if (iterK != iterG->second.end())
			{
				std::wstring ret(iterK->second.front());
				_removeEscape(ret);
				return ret;
			}
		}
		return defVal;
	}

	void set(std::wstring const & group, std::wstring const & key, std::wstring const & val)
	{
		_data[group][key].clear();
		_data[group][key].push_back(_toEscape(val));
		_data[group][key].push_back(val);
	}

	void set(std::wstring const & group, std::wstring const & key, std::vector<std::wstring> const & val)
	{
		_data[group][key].clear();
		std::wostringstream woss;
		_setLocale(woss);
		for (auto iter = val.begin(); iter != val.end(); ++iter)
		{
			woss << L"//" << *iter;
			_data[group][key].push_back(_toEscape(*iter));
		}
		_data[group][key].insert(_data[group][key].begin(), woss.str().c_str() + 2);
	}

	void clear()
	{
		_data.clear();
	}

	void removeGroup(std::wstring const & group)
	{
		_data.erase(group);
	}

private:
	std::wstring _trim(std::wstring const & str)
	{
		size_t start = 0;
		size_t end = str.length();
		for (size_t i = 0; i < end; ++i)
		{
			if (!std::isspace(str.at(i), _loc))
			{
				start = i;
				break;
			}
		}
		for (size_t i = end; i > 0; --i)
		{
			if (!std::isspace(str.at(i - 1), _loc))
			{
				end = i;
				break;
			}
		}
		if (start >= end)
			return std::wstring();
		else
			return str.substr(start, end - start);
	}

	std::wstring _removeComments(std::wstring const & str)
	{
		size_t end = str.length();
		for (size_t i = 0; i < end; ++i)
		{
			if (str.at(i) == L';')
			{
				end = i;
				break;
			}
		}
		if (end == 0)
			return std::wstring();
		else
			return str.substr(0, end);
	}

	void _insertData(std::wstring& group, std::wstring const & str)
	{
		size_t start = str.find_first_of(L'[');
		if (start != std::wstring::npos)
		{
			size_t end = str.find_last_of(L']');
			if (end != std::wstring::npos)
			{
				// group
				group = _trim(str.substr(start + 1, end - start - 1));
				return;
			}
		}
		// key - value
		size_t pos = str.find(L'=');
		if (pos == std::wstring::npos)
			return;
		std::wstring key = _trim(str.substr(0, pos));
		std::wstring value = _trim(str.substr(pos + 1));
		_data[group][key].push_back(value);
		_tokenize(value, _data[group][key]);
	}

	void _tokenize(std::wstring const & str, std::vector<std::wstring>& outVec)
	{
		size_t p = 0;
		size_t end = str.length();
		size_t find = std::wstring::npos;
		while (p < end)
		{
			find = str.find(L"//", p);
			if (find != std::wstring::npos)
			{
				outVec.push_back(_trim(str.substr(p, find - p)));
				p = find + 2;
			}
			else
			{
				outVec.push_back(_trim(str.substr(p, end - p)));
				break;
			}
		}

		// \\/ 이스케이프처리
		for (auto iterV = outVec.begin() + 1; iterV != outVec.end(); ++iterV)
		{
			_removeEscape(*iterV);
		}
	}

	template<class T>
	void _setLocale(T& target)
	{
		target.imbue(_loc);
	}

	std::wstring _toEscape(std::wstring const & val) const
	{
		wchar_t* buf = new wchar_t[val.length() * 2 + 1];
		wchar_t* p = buf;
		const wchar_t* r = val.c_str();
		while (*r != L'\0')
		{
			if (*r == L'/')
			{
				*p++ = L'\\';
				*p++ = *r++;
			}
			else if (*r == L'\r')
			{
				*p++ = L'\\';
				*p++ = L'r';
				++r;
			}
			else if (*r == L'\n')
			{
				*p++ = L'\\';
				*p++ = L'n';
				++r;
			}
			else if (*r == L'\t')
			{
				*p++ = L'\\';
				*p++ = L't';
				++r;
			}
			else
			{
				*p++ = *r++;
			}
		}
		*p++ = L'\0';
		std::wstring ret(buf);
		delete [] buf;
		return ret;
	}

	void _removeEscape(std::wstring& val) const
	{
		for (auto iterS = val.begin(); iterS != val.end(); ++iterS)
		{
			if (*iterS == L'\\')
			{
				auto iterNext = iterS;
				++iterNext;
				if (iterNext != val.end())
				{
					if (*iterNext == L'/')
					{
						iterS = val.erase(iterS);
					}
					else if (*iterNext == L'r')
					{
						iterS = val.erase(iterS);
						*iterS = L'\r';
					}
					else if (*iterNext == L'n')
					{
						iterS = val.erase(iterS);
						*iterS = L'\n';
					}
					else if (*iterNext == L't')
					{
						iterS = val.erase(iterS);
						*iterS = L'\t';
					}
				}
			}
		}
	}

	std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> _data;
	std::locale		_loc;
};
