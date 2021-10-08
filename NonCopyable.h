#pragma once

class NonCopyable
{
protected:
	NonCopyable() {}
	~NonCopyable() {}
private:
	NonCopyable(NonCopyable const &);
	NonCopyable const & operator=(NonCopyable const &);
};
