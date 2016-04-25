// This is the main DLL file.

#include "stdafx.h"

#include "FindManager.NET.h"

using namespace FindManagerWrapper;


/*** conversions ******************************/

wstring cnv::stlstr(String^ str)
{
	wstring result;
	pin_ptr<const wchar_t> pchars = PtrToStringChars(str);
	return result.assign(pchars, str->Length);
}

/*** FindManagerNet **************************/

FindManagerNet::FindManagerNet()
: _collchanged(false)
, _finder(0)
, _fclient(0)
, _synContext(nullptr)
{
	try
	{
		_finder = new FindManager();
		_fclient = new FinderClient(this);
	}
	catch(std::exception& ex)
	{
		throw gcnew Exception(gcnew String(ex.what()));
	}

	_finder->SetServiceEventClientPtr(_fclient);
}

FindManagerNet::FindManagerNet(SynchronizationContext^ synContext)
: _collchanged(false)
, _finder(0)
, _fclient(0)
, _synContext(synContext)
{
	try
	{
		_finder = new FindManager();
		_fclient = new FinderClient(this);
	}
	catch(std::exception& ex)
	{
		throw gcnew Exception(gcnew String(ex.what()));
	}

	_finder->SetServiceEventClientPtr(_fclient);
}

FindManagerNet::~FindManagerNet()
{
	delete _finder;
	delete _fclient;
}

bool FindManagerNet::Init(String^ deviceType)
{
	return _finder->Init(_fclient, cnv::stlstr(deviceType));
}

bool FindManagerNet::Init()
{
	return _finder->Init(_fclient);
}

bool FindManagerNet::Start()
{
	return _finder->Start();
}

bool FindManagerNet::Stop()
{
	return _finder->Stop();
}

Collections::IEnumerator^ FindManagerNet::GetEnumerator()
{
	_collchanged = false;
	return gcnew EnumObj<FindManagerNet, DeviceNet, DeviceArrayIterator>(this);
}

array<DeviceNet^>^ FindManagerNet::GetCollection()
{
	if(_finder->IsCollectionEmpty())
		return nullptr;

	int devscount = _finder->GetCollectionCount();
	const DeviceArray* pvdevs = _finder->GetCollection();

	array<DeviceNet^>^ result = gcnew array<DeviceNet^>(devscount);

	DeviceArrayIterator dai = pvdevs->begin();

	for(int i = 0; i < devscount; ++i, ++dai)
		result[i] = gcnew DeviceNet(*dai);

	return result;
}

DeviceNet^ FindManagerNet::Device::get(int index)
{
	if(_finder->IsCollectionEmpty() || index < 0 || index >= _finder->GetCollectionCount())
		return nullptr;
	
	return gcnew DeviceNet(_finder->GetDevice(index));
}

int FindManagerNet::CollectionSize::get()
{
	return _finder->GetCollectionCount();
}

bool FindManagerNet::IsCollectionEmpty::get()
{
	return _finder->IsCollectionEmpty();
}

bool FindManagerNet::IsCollectionChanged::get()
{
	return _collchanged;
}

void FindManagerNet::IsCollectionChanged::set(bool isChanged)
{
	_collchanged = isChanged;
}

void FindManagerNet::ResetIterators(enumhelper<DeviceArrayIterator>& eh)
{
	eh.reset(_finder->GetCollectionBegin(), _finder->GetCollectionEnd());
}

long FindManagerNet::FindId::get()
{
	return _finder->GetFindId();
}

void FindManagerNet::OnStartFind::add(StartFindEvent^ delStart)
{
	_evStart += delStart;
}

void FindManagerNet::OnStartFind::remove(StartFindEvent^ delStart)
{
	_evStart -= delStart;
}

void FindManagerNet::OnStartFind::raise(Object^ sender, StartFindArgs^ e)
{
	if(_evStart != nullptr)
		_evStart->Invoke(sender, e);
}

void FindManagerNet::OnStopFind::add(StopFindEvent^ delStop)
{
	_evStop += delStop;
}

void FindManagerNet::OnStopFind::remove(StopFindEvent^ delStop)
{
	_evStop -= delStop;
}

void FindManagerNet::OnStopFind::raise(Object^ sender, StopFindArgs^ e)
{
	if(_evStop != nullptr)
	{
		if(_synContext != nullptr)
			_synContext->Post(gcnew SendOrPostCallback(this, &FindManagerNet::StopFindSyncHandler), e);
		else
			_evStop->Invoke(sender, e);
	}
}

void FindManagerNet::OnAddDevice::add(AddDeviceEvent^ delAdd)
{
	_evAdd += delAdd;
}

void FindManagerNet::OnAddDevice::remove(AddDeviceEvent^ delAdd)
{
	_evAdd -= delAdd;
}

void FindManagerNet::OnAddDevice::raise(Object^ sender, AddDeviceArgs^ e)
{
	_collchanged = true;

	if(_evAdd != nullptr)
	{
		if(_synContext != nullptr)
			_synContext->Post(gcnew SendOrPostCallback(this, &FindManagerNet::AddDeviceSyncHandler), e);
		else
			_evAdd->Invoke(sender, e);
	}
}

void FindManagerNet::OnRemoveDevice::add(RemoveDeviceEvent^ delRemove)
{
	_evRemove += delRemove;
}

void FindManagerNet::OnRemoveDevice::remove(RemoveDeviceEvent^ delRemove)
{
	_evRemove -= delRemove;
}

void FindManagerNet::OnRemoveDevice::raise(Object^ sender, RemoveDeviceArgs^ e)
{
	_collchanged = true;

	if(_evRemove != nullptr)
	{
		if(_synContext != nullptr)
			_synContext->Post(gcnew SendOrPostCallback(this, &FindManagerNet::RemoveDeviceSyncHandler), e);
		else
			_evRemove->Invoke(sender, e);
	}
}

void FindManagerNet::OnServiceVariableChanged::add(ServiceVariableChangedEvent^ delVar)
{
	_evVar += delVar;
}

void FindManagerNet::OnServiceVariableChanged::remove(ServiceVariableChangedEvent^ delVar)
{
	_evVar -= delVar;
}

void FindManagerNet::OnServiceVariableChanged::raise(Object^ sender, ServiceVariableChangedArgs^ e)
{
	if(_evVar != nullptr)
	{
		if(_synContext != nullptr)
			_synContext->Post(gcnew SendOrPostCallback(this, &FindManagerNet::ServiceVariableChangedSyncHandler), e);
		else
			_evVar->Invoke(sender, e);
	}
}

void FindManagerNet::OnServiceInstanceDied::add(ServiceInstanceDiedEvent^ delDied)
{
	_evDied += delDied;
}

void FindManagerNet::OnServiceInstanceDied::remove(ServiceInstanceDiedEvent^ delDied)
{
	_evDied -= delDied;
}

void FindManagerNet::OnServiceInstanceDied::raise(Object^ sender, ServiceInstanceDiedArgs^ e)
{
	if(_evDied != nullptr)
	{
		if(_synContext != nullptr)
			_synContext->Post(gcnew SendOrPostCallback(this, &FindManagerNet::ServiceInstanceDiedSyncHandler), e);
		else
			_evDied->Invoke(sender, e);
	}
}

void FindManagerNet::StopFindSyncHandler(Object^ param)
{
	_evStop->Invoke(this, static_cast<StopFindArgs^>(param));
}

void FindManagerNet::AddDeviceSyncHandler(Object^ param)
{
	_evAdd->Invoke(this, static_cast<AddDeviceArgs^>(param));
}

void FindManagerNet::RemoveDeviceSyncHandler(Object^ param)
{
	_evRemove->Invoke(this, static_cast<RemoveDeviceArgs^>(param));
}

void FindManagerNet::ServiceVariableChangedSyncHandler(Object^ param)
{
	_evVar->Invoke(this, static_cast<ServiceVariableChangedArgs^>(param));
}

void FindManagerNet::ServiceInstanceDiedSyncHandler(Object^ param)
{
	_evDied->Invoke(this, static_cast<ServiceInstanceDiedArgs^>(param));
}


// *********************************************
// static helper functions
// *********************************************

String^ FindManagerNet::GetErrorMessage(long hresult)
{
	return gcnew String(UPnPCpLib::GetErrorMessage(hresult).c_str());
}

unsigned short FindManagerNet::GetVariantType(String^ variableType)
{
	return UPnPCpLib::GetVariantType(cnv::stlstr(variableType));
}

String^ FindManagerNet::GetTypeDescription(unsigned short variantType)
{
	return gcnew String(UPnPCpLib::GetTypeDescr(variantType).c_str());
}

bool FindManagerNet::IsICSConnectionEnabled()
{
	return UPnPCpLib::IsICSConnEnabled();
}

bool FindManagerNet::CheckSystemServiceState(String^ serviceName, /*out*/SrvState% serviceState)
{
	srvstate sstate;
	bool result = UPnPCpLib::CheckServiceState(cnv::stlstr(serviceName), sstate);
	if(result)
		serviceState = (SrvState)sstate;
	return result;
}

bool FindManagerNet::ControlSSDPService(bool start)
{
	return UPnPCpLib::ControlSSDPService(start);
}

int FindManagerNet::CheckFirewallPortState(long portNumber, Protocol protocol)
{
	return UPnPCpLib::CheckFirewallPortState(portNumber, transport_protocol(static_cast<int>(protocol)));
}

bool FindManagerNet::ControlUPnPPorts(bool open)
{
	return UPnPCpLib::ControlUPnPPorts(open);
}

int FindManagerNet::DeviceTypeCount::get()
{
	return FindManager::TypesCount();
}

array<String^>^ FindManagerNet::DeviceTypes::get()
{
	array<String^>^ types = gcnew array<String^>(FindManager::TypesCount());
	for(int i = 0; i < types->Length; ++i)
		types[i] = gcnew String(FindManager::device_type[i]);
	return types;
}

String^ FindManagerNet::RootDeviceType::get()
{
	return gcnew String(FindManager::GetRootDeviceType().c_str());
}

String^ FindManagerNet::DeviceType::get(int index)
{
	if(index < 0 || index >= FindManager::TypesCount())
		return String::Empty;
	else
		return gcnew String(FindManager::device_type[index]);
}
