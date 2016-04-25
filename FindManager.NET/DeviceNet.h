#pragma once

#include "templates.h"

using namespace System;
using namespace System::Collections;
using namespace UPnPCpLib;

namespace FindManagerWrapper
{

// forward declaration
ref class DeviceNet;


public ref class ProcessDeviceArgs : public EventArgs
{
public:
	ProcessDeviceArgs(DeviceNet^ devn, Object^ param, int procid)
	{
		Device = devn;
		Param = param;
		ProcId = procid;
	}

	property DeviceNet^ Device;
	property Object^ Param;
	property int ProcId;
};


public delegate void ProcessDeviceEvent(Object^, ProcessDeviceArgs^);


public enum class EnumSwitch
{
	Devices,
	Services,
	Icons
};


public ref struct IconParamNet
{
	String ^Url, ^Mime;
	int Width, Height, Depth;

internal:
	IconParamNet(const IconParam& icoparam);
};


class DeviceNetClient : public IProcessDevice
{
public:
	explicit DeviceNetClient(DeviceNet^ devn);
	virtual ~DeviceNetClient();

	virtual void ProcessDevice(const Device* dev, void* param, int procid);

private:
	gcroot<DeviceNet^> _wrapper;
};


public ref class DeviceNet : public Collections::IEnumerable
{
internal:
	DeviceNet(const Device* dev);

public:
	~DeviceNet();

	property String^ UniqueName {String^ get();}
	property String^ FriendlyName {String^ get();}
	property String^ Type {String^ get();}
	property DeviceNet^ ParentDevice {DeviceNet^ get();}
	property DeviceNet^ RootDevice {DeviceNet^ get();}
	property bool IsRoot {bool get();}
	property String^ DocumentURL {String^ get();}
	property StringDictionary^ Info {StringDictionary^ get();}
	String^ GetDocumentContent();

	// access to collection of devices
	array<DeviceNet^>^ GetDevices();
	property DeviceNet^ Device[int] {DeviceNet^ get(int index);}
	property int DeviceCount {int get();}
	property bool IsDeviceCollectionEmpty {bool get();}

	// access to collection of services
	array<ServiceNet^>^ GetServices();
	property ServiceNet^ Service[int] {ServiceNet^ get(int index);}
	property int ServiceCount {int get();}

	// access to collection of icons
	array<IconParamNet^>^ GetIcons();
	property IconParamNet^ Icon[int] {IconParamNet^ get(int index);}
	property int IconCount {int get();}
	property bool IsIconCollectionEmpty {bool get();}

	String^ GetIconURL(int width, int height, int depth);

	// enumerator switch
	// for choosing the enumerated collection
	property EnumSwitch EnumeratorSwitch {EnumSwitch get(); void set(EnumSwitch enumval);}

	// common method for enumerate both collections,
	// which collection is currently enumerated depends on enumerator switch
	virtual Collections::IEnumerator^ GetEnumerator();

	event ProcessDeviceEvent^ OnProcessDevice
	{
	public:
		void add(ProcessDeviceEvent^ delProc);
		void remove(ProcessDeviceEvent^ delProc);
		void raise(Object^ sender, ProcessDeviceArgs^ e);
	}

	// processes root device structure
	// and for each member device OnProcessDevice event is raised
	// both arguments can be null
	void EnumerateDevices(Object^ param, int procid);

internal:
	property bool IsCollectionChanged {bool get(); void set(bool);}
	void ResetIterators(enumhelper<ServiceIterator>& eh);
	void ResetIterators(enumhelper<DeviceIterator>& eh);
	void ResetIterators(enumhelper<IconIterator>& eh);

private:
	const UPnPCpLib::Device* _dev;
	DeviceNetClient* _client;
	ProcessDeviceEvent^ _evProc;

	EnumSwitch _switch;
	// collections are never changing
	bool _collchanged;
};

}