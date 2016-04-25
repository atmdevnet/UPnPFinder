// Wrapper.h

#pragma once

#include "FinderClient.h"

using namespace System;
using namespace UPnPCpLib;

namespace Wrapper
{

public delegate void StartFindEvent();
public delegate void StopFindEvent(bool);
public delegate void AddDeviceEvent(/*const Device* dev, */int);
public delegate void RemoveDeviceEvent(String^, String^, int);

public ref class FinderWrapper
{
public:
	FinderWrapper();
	~FinderWrapper();

	bool Start();
	bool Stop();

	event StartFindEvent^ OnStartFind
	{
	public:
		void add(StartFindEvent^ delStart);
		void remove(StartFindEvent^ delStart);
		void raise();
	}
	event StopFindEvent^ OnStopFind
	{
	public:
		void add(StopFindEvent^ delStop);
		void remove(StopFindEvent^ delStop);
		void raise(bool iscancelled);
	}
	event AddDeviceEvent^ OnAddDevice
	{
	public:
		void add(AddDeviceEvent^ delAdd);
		void remove(AddDeviceEvent^ delAdd);
		void raise(int devindex);
	}
	event RemoveDeviceEvent^ OnRemoveDevice
	{
	public:
		void add(RemoveDeviceEvent^ delRemove);
		void remove(RemoveDeviceEvent^ delRemove);
		void raise(String^ devudn, String^ friendlyname, int removedindex);
	}

private:
	FindManager* _finder;
	FinderClient* _fclient;

	StartFindEvent^ _evStart;
	StopFindEvent^ _evStop;
	AddDeviceEvent^ _evAdd;
	RemoveDeviceEvent^ _evRemove;
};

}
