#pragma once

using namespace System;

namespace FindManagerWrapper
{

template<class TcollIter>
class enumhelper
{
public:
	TcollIter operator++() {return ++_current;}
	operator bool () {return _current != _end;}
	void reset(TcollIter beg, TcollIter end) {_current = beg; _end = end;}
	TcollIter get_current() {return _current;}

private:
	TcollIter _current, _end;
};


template<class Thost, class TcollType, class TcollIter>
ref class EnumObj : public Collections::IEnumerator
{
public:
	EnumObj(Thost^ host)
	: _host(host)
	, _first(true)
	, _e(new enumhelper<TcollIter>())
	{
		_host->ResetIterators(*_e);
	}

	~EnumObj()
	{
		delete _e;
	}

	virtual bool MoveNext()
	{
		if(_host->IsCollectionChanged)
			return false;

		if(_first)
			_first = false;
		else
			++(*_e);

		return *_e;
	}

	virtual void Reset()
	{
		_host->IsCollectionChanged = false;
		_host->ResetIterators(*_e);
		_first = true;
	}

	virtual property Object^ Current
	{
		Object^ get()
		{
			if(_host->IsCollectionChanged || _first)
				return nullptr;
			else
				return gcnew TcollType(*_e->get_current());
		}
	}

private:
	Thost^ _host;
	enumhelper<TcollIter>* _e;
	bool _first;
};

}