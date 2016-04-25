#pragma once

#include "templates.h"

using namespace System;
using namespace System::Collections;
using namespace System::Collections::Specialized;
using namespace System::Collections::Generic;
using namespace UPnPCpLib;

namespace FindManagerWrapper
{

// forward declaration
ref class DeviceNet;
ref class ServiceNet;

typedef KeyValuePair<String^, String^> ArgData;
typedef List<ArgData> ArgsList;

typedef KeyValuePair<String^, bool> StateVarData;
typedef List<StateVarData> StateVarList;



public ref class ActionNet
{
internal:
	ActionNet(const UPnPCpLib::Action& act);

public:
	property String^ Name {String^ get();}
	property int ArgumentCount {int get();}
	property int ArgumentInCount {int get();}
	property List<String^>^ ArgumentsIn {void set(List<String^>^);}
	property String^ Argument[int] {void set(int index, String^ argValue);}
	property ArgsList^ Arguments {ArgsList^ get();}
	property List<ArgsList^>^ Info {List<ArgsList^>^ get();}
	property ServiceNet^ ParentService {ServiceNet^ get();}

	int Invoke(ArgsList^% outArgs);

private:
	const UPnPCpLib::Action& _act;
};


public ref class ServiceNet : public Collections::IEnumerable
{
internal:
	ServiceNet(const Service* srv);
	~ServiceNet();

public:
	virtual Collections::IEnumerator^ GetEnumerator();
	array<ActionNet^>^ GetActions();
	property ActionNet^ Action[int] {ActionNet^ get(int index);}
	property ActionNet^ Action[String^] {ActionNet^ get(String^ actionName);}
	property int ActionsCount {int get();}

	property StateVarList^ StateVariables {StateVarList^ get();}

	property StringDictionary^ Info {StringDictionary^ get();}
	property String^ ServiceID {String^ get();}
	property String^ ServiceTypeID {String^ get();}
	property long LastTransportStatus {long get();}

	property String^ ScpdURL {String^ get();}
	String^ GetScpdContent();

	property DeviceNet^ ParentDevice {DeviceNet^ get();}

internal:
	property bool IsCollectionChanged {bool get(); void set(bool);}
	void ResetIterators(enumhelper<ActionIterator>& eh);

private:
	const Service* _srv;
	
	// collection is never changing
	bool _collchanged;
};

}