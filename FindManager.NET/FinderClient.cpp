#include "StdAfx.h"

#include "FinderClient.h"
#include "FindManager.NET.h"
#include "servicenet.h"
#include "DeviceNet.h"

using namespace FindManagerWrapper;

FinderClient::FinderClient(FindManagerNet^ fmn)
: _wrapper(fmn)
{
}

FinderClient::~FinderClient()
{
	_wrapper = nullptr;
}

void FinderClient::OnStartFindDevice(long findid)
{
	_wrapper->OnStartFind(static_cast<FindManagerNet^>(_wrapper), gcnew StartFindArgs(findid));
}

void FinderClient::OnStopFindDevice(long findid, bool iscancelled)
{
	_wrapper->OnStopFind(static_cast<FindManagerNet^>(_wrapper), gcnew StopFindArgs(findid, iscancelled));
}

void FinderClient::OnAddDevice(long findid, const Device* dev, int devindex)
{
	_wrapper->OnAddDevice(static_cast<FindManagerNet^>(_wrapper), gcnew AddDeviceArgs(findid, gcnew DeviceNet(dev), devindex));
}

void FinderClient::OnRemoveDevice(long findid, const wstring& devudn, const wstring& friendlyname, int removedindex)
{
	_wrapper->OnRemoveDevice(static_cast<FindManagerNet^>(_wrapper), gcnew RemoveDeviceArgs(findid, gcnew String(devudn.c_str()), gcnew String(friendlyname.c_str()), removedindex));
}

void FinderClient::ServiceEventVariableChanged(const Service* srv, const wstring& varname, const wstring& varvalue)
{
	_wrapper->OnServiceVariableChanged(static_cast<FindManagerNet^>(_wrapper), gcnew ServiceVariableChangedArgs(gcnew ServiceNet(srv), gcnew String(varname.c_str()), gcnew String(varvalue.c_str())));
}

void FinderClient::ServiceEventInstanceDied(const Service* srv)
{
	_wrapper->OnServiceInstanceDied(static_cast<FindManagerNet^>(_wrapper), gcnew ServiceInstanceDiedArgs(gcnew ServiceNet(srv)));
}
