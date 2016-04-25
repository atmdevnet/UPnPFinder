#pragma once

using namespace System;
using namespace UPnPCpLib;

namespace FindManagerWrapper
{

// forward declaration
ref class FindManagerNet;

class FinderClient : public IFinderManagerClient, public IServiceCallbackClient
{
public:
	explicit FinderClient(FindManagerNet^ fmn);
	virtual ~FinderClient();

	virtual void OnStartFindDevice(long findid);
	virtual void OnStopFindDevice(long findid, bool iscancelled);
	virtual void OnAddDevice(long findid, const Device* dev, int devindex);
	virtual void OnRemoveDevice(long findid, const wstring& devudn, const wstring& friendlyname, int removedindex);

	virtual void ServiceEventVariableChanged(const Service* srv, const wstring& varname, const wstring& varvalue);
	virtual void ServiceEventInstanceDied(const Service* srv);

private:
	gcroot<FindManagerNet^> _wrapper;
};

}
