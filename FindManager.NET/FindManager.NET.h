#pragma once

#include "FinderClient.h"
#include "ServiceNet.h"
#include "DeviceNet.h"
#include "templates.h"

using namespace System;
using namespace System::Collections;
using namespace System::Collections;
using namespace System::Threading;
using namespace UPnPCpLib;

namespace FindManagerWrapper
{

/*** events args classes **********************/

public ref class StartFindArgs : public EventArgs
{
public:
	StartFindArgs(long findId)
	{
		FindId = findId;
	}

	property long FindId;
};

public ref class StopFindArgs : public EventArgs
{
public:
	StopFindArgs(long findId, bool isCancel)
	{
		FindId = findId;
		IsCancelled = isCancel;
	}

	property long FindId;
	property bool IsCancelled;
};

public ref class AddDeviceArgs : public EventArgs
{
public:
	AddDeviceArgs(long findId, DeviceNet^ devn, int index)
	{
		FindId = findId;
		Device = devn;
		Index = index;
	}

	property long FindId;
	property DeviceNet^ Device;
	property int Index;
};

public ref class RemoveDeviceArgs : public EventArgs
{
public:
	RemoveDeviceArgs(long findId, String^ udn, String^ friendlyName, int removedIndex)
	{
		FindId = findId;
		UDN = udn;
		FriendlyName = friendlyName;
		RemovedIndex = removedIndex;
	}

	property long FindId;
	property String^ UDN;
	property String^ FriendlyName;
	property int RemovedIndex;
};

public ref class ServiceVariableChangedArgs : public EventArgs
{
public:
	ServiceVariableChangedArgs(ServiceNet^ srvn, String^ varName, String^ varValue)
	{
		Service = srvn;
		VarName = varName;
		VarValue = varValue;
	}

	property ServiceNet^ Service;
	property String^ VarName;
	property String^ VarValue;
};

public ref class ServiceInstanceDiedArgs : public EventArgs
{
public:
	ServiceInstanceDiedArgs(ServiceNet^ srvn)
	{
		Service = srvn;
	}

	property ServiceNet^ Service;
};


/*** delegates for events *********************/

public delegate void StartFindEvent(Object^, StartFindArgs^);
public delegate void StopFindEvent(Object^, StopFindArgs^);
public delegate void AddDeviceEvent(Object^, AddDeviceArgs^);
public delegate void RemoveDeviceEvent(Object^, RemoveDeviceArgs^);

public delegate void ServiceVariableChangedEvent(Object^, ServiceVariableChangedArgs^);
public delegate void ServiceInstanceDiedEvent(Object^, ServiceInstanceDiedArgs^);


/*** conversions ******************************/

private ref class cnv
{
public:
	static wstring stlstr(String^ str);
};


/*** state of system services *****************/

public enum class SrvState
{
	StartPending,		
	StopPending,		
	PausePending,		
	ContinuePending,	
	Running,			
	Stopped,			
	Paused				
};


/*** protocols identifiers ********************/

public enum class Protocol
{
	Any,
	TCP,
	UDP
};


public ref class FindManagerNet : public Collections::IEnumerable
{
public:
	FindManagerNet();
	FindManagerNet(SynchronizationContext^ synContext);
	~FindManagerNet();

	bool Init(String^ deviceType);
	bool Init();
	bool Start();
	bool Stop();

	virtual Collections::IEnumerator^ GetEnumerator();
	array<DeviceNet^>^ GetCollection();
	property DeviceNet^ Device[int] {DeviceNet^ get(int index);}
	property int CollectionSize	{int get();}
	property bool IsCollectionEmpty	{bool get();}
	property long FindId {long get();}

internal:
	property bool IsCollectionChanged {bool get(); void set(bool);}
	void ResetIterators(enumhelper<DeviceArrayIterator>& eh);

public:
	event StartFindEvent^ OnStartFind
	{
	public:
		void add(StartFindEvent^ delStart);
		void remove(StartFindEvent^ delStart);
		void raise(Object^ sender, StartFindArgs^ e);
	}
	event StopFindEvent^ OnStopFind
	{
	public:
		void add(StopFindEvent^ delStop);
		void remove(StopFindEvent^ delStop);
		void raise(Object^ sender, StopFindArgs^ e);
	}
	event AddDeviceEvent^ OnAddDevice
	{
	public:
		void add(AddDeviceEvent^ delAdd);
		void remove(AddDeviceEvent^ delAdd);
		void raise(Object^ sender, AddDeviceArgs^ e);
	}
	event RemoveDeviceEvent^ OnRemoveDevice
	{
	public:
		void add(RemoveDeviceEvent^ delRemove);
		void remove(RemoveDeviceEvent^ delRemove);
		void raise(Object^ sender, RemoveDeviceArgs^ e);
	}

	event ServiceVariableChangedEvent^ OnServiceVariableChanged
	{
	public:
		void add(ServiceVariableChangedEvent^ delVar);
		void remove(ServiceVariableChangedEvent^ delVar);
		void raise(Object^ sender, ServiceVariableChangedArgs^ e);
	}
	event ServiceInstanceDiedEvent^ OnServiceInstanceDied
	{
	public:
		void add(ServiceInstanceDiedEvent^ delDied);
		void remove(ServiceInstanceDiedEvent^ delDied);
		void raise(Object^ sender, ServiceInstanceDiedArgs^ e);
	}

	//
	// static helper functions
	//

	// converts hresult of action's invokation to message string.
	// HRESULT to string
	static String^ GetErrorMessage(long hresult);

	// converts description of state variable type to variant type
	// string to VARTYPE
	static unsigned short GetVariantType(String^ variableType);

	// converts variant type to description of state variable type
	// VARTYPE to string
	static String^ GetTypeDescription(unsigned short variantType);

	// checks for Internet Connection Sharing enabled connections
	static bool IsICSConnectionEnabled();

	// checks system service state
	static bool CheckSystemServiceState(String^ serviceName, /*out*/SrvState% serviceState);

	// starts or stops ssdp system service
	static bool ControlSSDPService(bool start);

	// checks specified firewall port state
	// returns: 0 - error, 1 - enabled, -1 - disabled
	static int CheckFirewallPortState(long portNumber, Protocol protocol);

	// opens or closes UPnP ports (2869 TCP, 1900 UDP)
	static bool ControlUPnPPorts(bool open);

	//
	// static UPnP devices types
	//
	static property int DeviceTypeCount {int get();}
	static property array<String^>^ DeviceTypes {array<String^>^ get();}
	static property String^ RootDeviceType {String^ get();}
	static property String^ DeviceType[int] {String^ get(int index);}

private:
	void StopFindSyncHandler(Object^ param);
	void AddDeviceSyncHandler(Object^ param);
	void RemoveDeviceSyncHandler(Object^ param);
	void ServiceVariableChangedSyncHandler(Object^ param);
	void ServiceInstanceDiedSyncHandler(Object^ param);

private:
	FindManager* _finder;
	FinderClient* _fclient;

	bool _collchanged;

	StartFindEvent^ _evStart;
	StopFindEvent^ _evStop;
	AddDeviceEvent^ _evAdd;
	RemoveDeviceEvent^ _evRemove;

	ServiceVariableChangedEvent^ _evVar;
	ServiceInstanceDiedEvent^ _evDied;

	SynchronizationContext^ _synContext;
};

}
