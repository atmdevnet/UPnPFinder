#include "StdAfx.h"
#include "ServiceNet.h"
#include "DeviceNet.h"
#include "FindManager.NET.h"

using namespace FindManagerWrapper;


/*** IconParamNet *****************************/

IconParamNet::IconParamNet(const IconParam& icoparam)
: Url(gcnew String(icoparam._url.c_str()))
, Mime(gcnew String(icoparam._mime.c_str()))
, Width(icoparam._width)
, Height(icoparam._height)
, Depth(icoparam._depth)
{
}

/*** DeviceNetClient **************************/

DeviceNetClient::DeviceNetClient(DeviceNet^ devn)
: _wrapper(devn)
{
}

DeviceNetClient::~DeviceNetClient()
{
	_wrapper = nullptr;
}

void DeviceNetClient::ProcessDevice(const Device* dev, void* param, int procid)
{
	pin_ptr<void> pparam = param;
	interior_ptr<Object^> iparam = reinterpret_cast<interior_ptr<Object^>>(pparam);

	_wrapper->OnProcessDevice(static_cast<DeviceNet^>(_wrapper), gcnew ProcessDeviceArgs(gcnew DeviceNet(dev), *iparam, procid));
}

/*** DeviceNet *******************************/

DeviceNet::DeviceNet(const UPnPCpLib::Device* dev)
: _dev(dev)
, _collchanged(false)
, _switch(EnumSwitch::Devices)
{
	_client = new DeviceNetClient(this);
}

DeviceNet::~DeviceNet()
{
	delete _client;
}

String^ DeviceNet::UniqueName::get()
{
	return gcnew String(_dev->GetUDN().c_str());
}

String^ DeviceNet::FriendlyName::get()
{
	return gcnew String(_dev->GetFriendlyName().c_str());
}

String^ DeviceNet::Type::get()
{
	return gcnew String(_dev->GetType().c_str());
}

DeviceNet^ DeviceNet::ParentDevice::get()
{
	return _dev->IsRoot() ? nullptr : gcnew DeviceNet(_dev->GetParentDevice());
}

DeviceNet^ DeviceNet::RootDevice::get()
{
	return gcnew DeviceNet(_dev->GetRootDevice());
}

bool DeviceNet::IsRoot::get()
{
	return _dev->IsRoot();
}

String^ DeviceNet::DocumentURL::get()
{
	return gcnew String(_dev->GetDevDocAccessURL().c_str());
}

String^ DeviceNet::GetDocumentContent()
{
	return gcnew String(_dev->GetAccessData()->_doc.c_str());
}

StringDictionary^ DeviceNet::Info::get()
{
	StringDictionary^ result = nullptr;

	InfoData devinfo;
	
	if(_dev->GetDeviceInfo(devinfo))
	{
		result = gcnew StringDictionary;

		for(InfoIterator ii = devinfo.begin(); ii != devinfo.end(); ++ii)
			result->Add(gcnew String((*ii).first.c_str()), gcnew String((*ii).second.c_str()));
	}

	return result;
}

array<DeviceNet^>^ DeviceNet::GetDevices()
{
	if(_dev->IsDeviceListEmpty())
		return nullptr;

	int devscount = _dev->GetDeviceListCount();

	array<DeviceNet^>^ result = gcnew array<DeviceNet^>(devscount);

	DeviceIterator di = _dev->GetDeviceListBegin();

	for(int i = 0; i < devscount; ++i, ++di)
		result[i] = gcnew DeviceNet(*di);

	return result;
}

DeviceNet^ DeviceNet::Device::get(int index)
{
	if(_dev->IsDeviceListEmpty() || index >= _dev->GetDeviceListCount() || index < 0)
		return nullptr;

	return gcnew DeviceNet(_dev->GetDevice(index));
}

int DeviceNet::DeviceCount::get()
{
	return _dev->GetDeviceListCount();
}

bool DeviceNet::IsDeviceCollectionEmpty::get()
{
	return _dev->IsDeviceListEmpty();
}

array<ServiceNet^>^ DeviceNet::GetServices()
{
	int srvcount = _dev->GetServiceListCount();

	array<ServiceNet^>^ result = gcnew array<ServiceNet^>(srvcount);

	ServiceIterator si = _dev->GetServiceListBegin();

	for(int i = 0; i < srvcount; ++i, ++si)
		result[i] = gcnew ServiceNet(*si);

	return result;
}

ServiceNet^ DeviceNet::Service::get(int index)
{
	if(index >= _dev->GetServiceListCount() || index < 0)
		return nullptr;

	return gcnew ServiceNet(_dev->GetService(index));
}

int DeviceNet::ServiceCount::get()
{
	return _dev->GetServiceListCount();
}

array<IconParamNet^>^ DeviceNet::GetIcons()
{
	if(_dev->IsIconsListEmpty())
		return nullptr;

	int icocount = _dev->GetIconsListCount();

	array<IconParamNet^>^ result = gcnew array<IconParamNet^>(icocount);

	IconIterator ii = _dev->GetIconsListBegin();

	for(int i = 0; i < icocount; ++i, ++ii)
		result[i] = gcnew IconParamNet(*ii);

	return result;
}

IconParamNet^ DeviceNet::Icon::get(int index)
{
	if(_dev->IsIconsListEmpty() || index >= _dev->GetIconsListCount() || index < 0)
		return nullptr;

	return gcnew IconParamNet(_dev->GetIcon(index));
}

int DeviceNet::IconCount::get()
{
	return _dev->GetIconsListCount();
}

bool DeviceNet::IsIconCollectionEmpty::get()
{
	return _dev->IsIconsListEmpty();
}

String^ DeviceNet::GetIconURL(int width, int height, int depth)
{
	return gcnew String(_dev->GetRootDevice()->GetDeviceIconURL(width, height, depth).c_str());
}

EnumSwitch DeviceNet::EnumeratorSwitch::get()
{
	return _switch;
}

void DeviceNet::EnumeratorSwitch::set(EnumSwitch enumval)
{
	_switch = enumval;
}

Collections::IEnumerator^ DeviceNet::GetEnumerator()
{
	_collchanged = false;

	switch(_switch)
	{
	case EnumSwitch::Services :
		return gcnew EnumObj<DeviceNet, ServiceNet, ServiceIterator>(this);
	case EnumSwitch::Icons :
		return gcnew EnumObj<DeviceNet, IconParamNet, IconIterator>(this);
	default:
		return gcnew EnumObj<DeviceNet, DeviceNet, DeviceIterator>(this);
	}
}

void DeviceNet::OnProcessDevice::add(ProcessDeviceEvent^ delProc)
{
	_evProc += delProc;
}

void DeviceNet::OnProcessDevice::remove(ProcessDeviceEvent^ delProc)
{
	_evProc -= delProc;
}

void DeviceNet::OnProcessDevice::raise(Object^ sender, ProcessDeviceArgs^ e)
{
	_evProc->Invoke(sender, e);
}

void DeviceNet::EnumerateDevices(Object^ param, int procid)
{
	interior_ptr<Object^> iparam = &param;
	pin_ptr<void> pparam = iparam;

	_dev->EnumerateDevices(_client, pparam, procid);
}

bool DeviceNet::IsCollectionChanged::get()
{
	return _collchanged;
}

void DeviceNet::IsCollectionChanged::set(bool isChanged)
{
	_collchanged = isChanged;
}

void DeviceNet::ResetIterators(enumhelper<ServiceIterator>& eh)
{
	eh.reset(_dev->GetServiceListBegin(), _dev->GetServiceListEnd());
}

void DeviceNet::ResetIterators(enumhelper<DeviceIterator>& eh)
{
	eh.reset(_dev->GetDeviceListBegin(), _dev->GetDeviceListEnd());
}

void DeviceNet::ResetIterators(enumhelper<IconIterator>& eh)
{
	eh.reset(_dev->GetIconsListBegin(), _dev->GetIconsListEnd());
}

