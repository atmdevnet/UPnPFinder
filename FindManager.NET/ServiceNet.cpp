#include "StdAfx.h"
#include "ServiceNet.h"
#include "DeviceNet.h"
#include "FindManager.NET.h"

using namespace FindManagerWrapper;

ActionNet::ActionNet(const UPnPCpLib::Action& act)
: _act(act)
{
}

String^ ActionNet::Name::get()
{
	return gcnew String(_act.GetName().c_str());
}

int ActionNet::ArgumentCount::get()
{
	return const_cast<UPnPCpLib::Action&>(_act).GetArgsCount();
}

int ActionNet::ArgumentInCount::get()
{
	return const_cast<UPnPCpLib::Action&>(_act).GetInArgsCount();
}

void ActionNet::ArgumentsIn::set(List<String^>^ args)
{
	StrList arglist;

	for each(String^ arg in args)
		arglist.push_back(cnv::stlstr(arg));

	const_cast<UPnPCpLib::Action&>(_act).SetInArgs(arglist);
}

void ActionNet::Argument::set(int index, String^ argValue)
{
	const_cast<UPnPCpLib::Action&>(_act).SetInArgs(cnv::stlstr(argValue), index);
}

ArgsList^ ActionNet::Arguments::get()
{
	ArgsList^ result = nullptr;

	ArgsArray args;
	if(const_cast<UPnPCpLib::Action&>(_act).GetInArgs(args))
	{
		result = gcnew ArgsList();

		for(ArgsIterator ai = args.begin(); ai != args.end(); ++ai)
			result->Add(ArgData(gcnew String((*ai).first.c_str()), gcnew String((*ai).second.c_str())));
	}

	return result;
}

List<ArgsList^>^ ActionNet::Info::get()
{
	List<ArgsList^>^ result = nullptr;

	InfoDataList argsinfo;
	
	if(_act.GetInfo(argsinfo))
	{
		result = gcnew List<ArgsList^>();

		InfoDataList::const_iterator argi;

		for(argi = argsinfo.begin(); argi != argsinfo.end(); ++argi)
		{
			ArgsList^ arglist = gcnew ArgsList();

			for(InfoIterator ii = (*argi).begin(); ii != (*argi).end(); ++ii)
				arglist->Add(ArgData(gcnew String((*ii).first.c_str()), gcnew String((*ii).second.c_str())));

			result->Add(arglist);
		}
	}

	return result;
}

ServiceNet^ ActionNet::ParentService::get()
{
	return gcnew ServiceNet(&_act.GetParentService());
}

int ActionNet::Invoke(ArgsList^% outArgs)
{
	ArgsArray argsout;

	int result = _act.Invoke(argsout);

	// copy out args
	for(ArgsIterator ai = argsout.begin(); ai != argsout.end(); ++ai)
		outArgs->Add(ArgData(gcnew String((*ai).first.c_str()), gcnew String((*ai).second.c_str())));

	return result;
}


ServiceNet::ServiceNet(const Service* srv)
: _srv(srv)
, _collchanged(false)
{
}

ServiceNet::~ServiceNet()
{
}

Collections::IEnumerator^ ServiceNet::GetEnumerator()
{
	_collchanged = false;
	return gcnew EnumObj<ServiceNet, ActionNet, ActionIterator>(this);
}

array<ActionNet^>^ ServiceNet::GetActions()
{
	int actcount = _srv->GetActionCount();
	ActionIterator ai = _srv->GetCollectionBegin();

	array<ActionNet^>^ actions = gcnew array<ActionNet^>(actcount);

	for(int i = 0; i < actcount; ++i, ++ai)
		actions[i] = gcnew ActionNet(*ai);

	return actions;
}

ActionNet^ ServiceNet::Action::get(int index)
{
	if(index < 0 || index >= _srv->GetActionCount())
		return nullptr;

	return gcnew ActionNet(_srv->GetAction(index));
}

ActionNet^ ServiceNet::Action::get(String^ actionName)
{
	ActionNet^ result;

	try
	{
		result = gcnew ActionNet(_srv->GetAction(cnv::stlstr(actionName)));
	}
	catch(invalid_argument)
	{
		result = nullptr;
	}

	return result;
}

int ServiceNet::ActionsCount::get()
{
	return _srv->GetActionCount();
}

StateVarList^ ServiceNet::StateVariables::get()
{
	StateVarList^ result = nullptr;

	VarData vars;
	
	if(_srv->GetServiceVariables(vars))
	{
		result = gcnew StateVarList();

		for(VarIterator vi = vars.begin(); vi != vars.end(); ++vi)
			result->Add(StateVarData(gcnew String((*vi).first.c_str()), (*vi).second));
	}

	return result;
}

StringDictionary^ ServiceNet::Info::get()
{
	StringDictionary^ result = nullptr;

	InfoData srvinfo;
	
	if(_srv->GetServiceInfo(srvinfo))
	{
		result = gcnew StringDictionary;

		for(InfoIterator ii = srvinfo.begin(); ii != srvinfo.end(); ++ii)
			result->Add(gcnew String((*ii).first.c_str()), gcnew String((*ii).second.c_str()));
	}

	return result;
}

String^ ServiceNet::ServiceID::get()
{
	return gcnew String(_srv->GetServiceID().c_str());
}

String^ ServiceNet::ServiceTypeID::get()
{
	return gcnew String(_srv->GetServiceTypeID().c_str());
}

long ServiceNet::LastTransportStatus::get()
{
	return _srv->GetLastTransportStatus();
}

String^ ServiceNet::ScpdURL::get()
{
	return gcnew String(_srv->GetScpdURL().c_str());
}

String^ ServiceNet::GetScpdContent()
{
	return gcnew String(_srv->GetScpdContent().c_str());
}

DeviceNet^ ServiceNet::ParentDevice::get()
{
	return gcnew DeviceNet(&_srv->GetParentDevice());
}

bool ServiceNet::IsCollectionChanged::get()
{
	return _collchanged;
}

void ServiceNet::IsCollectionChanged::set(bool isChanged)
{
	_collchanged = isChanged;
}

void ServiceNet::ResetIterators(enumhelper<ActionIterator>& eh)
{
	eh.reset(_srv->GetCollectionBegin(), _srv->GetCollectionEnd());
}
