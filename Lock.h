#pragma once

class LockCS
{
public:
	LockCS()
	{
		_bInit = (InitializeCriticalSectionAndSpinCount(&_cs, 2000) == TRUE);
	}

	~LockCS()
	{
		if (_bInit)
		{
			DeleteCriticalSection(&_cs);
			_bInit = false;
		}
	}

	void lock()
	{
		if (!_bInit)
			throw 0;
		EnterCriticalSection(&_cs);
	}

	void unlock()
	{
		if (!_bInit)
			throw 0;
		LeaveCriticalSection(&_cs);
	}

private:
	bool				_bInit;
	CRITICAL_SECTION	_cs;
};

template <class TLock>
class Lock
{
public:
	Lock(TLock* lockObj)
		: _lockObj(lockObj)
	{
		_lockObj->lock();
	}

	~Lock()
	{
		_lockObj->unlock();
	}

	operator bool()
	{
		return false;
	}

private:
	TLock*	_lockObj;
};

#define SYNC(lockobj) if (Lock<LockCS> _lockguard##__LINE__ = (&lockobj)) {} else
