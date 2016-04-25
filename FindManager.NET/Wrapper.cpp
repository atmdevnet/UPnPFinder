// This is the main DLL file.

#include "stdafx.h"

#include "Wrapper.h"

using namespace Wrapper;

FinderWrapper::FinderWrapper()
: _finder(new FindManager())
{
	_fclient = new FinderClient(this);
	_finder->CreateCallback();
	_finder->SetClientPtr(_fclient);
}

FinderWrapper::~FinderWrapper()
{
	delete _finder;
	delete _fclient;
}

bool FinderWrapper::Start()
{
	return _finder->Start();
}

bool FinderWrapper::Stop()
{
	return _finder->Stop();
}

void FinderWrapper::OnStartFind::add(StartFindEvent^ delStart)
{
	_evStart += delStart;
}

void FinderWrapper::OnStartFind::remove(StartFindEvent^ delStart)
{
	_evStart -= delStart;
}

void FinderWrapper::OnStartFind::raise()
{
	_evStart->Invoke();
}

void FinderWrapper::OnStopFind::add(StopFindEvent^ delStop)
{
	_evStop += delStop;
}

void FinderWrapper::OnStopFind::remove(StopFindEvent^ delStop)
{
	_evStop -= delStop;
}

void FinderWrapper::OnStopFind::raise(bool iscancelled)
{
	_evStop->Invoke(iscancelled);
}

void FinderWrapper::OnAddDevice::add(AddDeviceEvent^ delAdd)
{
	_evAdd += delAdd;
}

void FinderWrapper::OnAddDevice::remove(AddDeviceEvent^ delAdd)
{
	_evAdd -= delAdd;
}

void FinderWrapper::OnAddDevice::raise(int devindex)
{
	_evAdd->Invoke(devindex);
}

void FinderWrapper::OnRemoveDevice::add(RemoveDeviceEvent^ delRemove)
{
	_evRemove += delRemove;
}

void FinderWrapper::OnRemoveDevice::remove(RemoveDeviceEvent^ delRemove)
{
	_evRemove -= delRemove;
}

void FinderWrapper::OnRemoveDevice::raise(String^ devudn, String^ friendlyname, int removedindex)
{
	_evRemove->Invoke(devudn, friendlyname, removedindex);
}

