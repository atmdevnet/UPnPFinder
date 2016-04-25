#include "upnpcplib.h"

using namespace UPnPCpLib;


// *********************************************
// IconParam class
// *********************************************


IconParam::IconParam()
: _width(0)
, _height(0)
, _depth(0)
{}

IconParam::IconParam(const wstring& vurl, const wstring& vmime, int vwidth, int vheight, int vdepth) 
: _url(vurl)
, _mime(vmime)
, _width(vwidth)
, _height(vheight)
, _depth(vdepth)
{}

IconParam::IconParam(const IconParam& srcobj)
: _url(srcobj._url)
, _mime(srcobj._mime)
, _width(srcobj._width)
, _height(srcobj._height)
, _depth(srcobj._depth)
{}

IconParam& IconParam::operator= (const IconParam& srcobj)
{
	if(&srcobj == this)
		return *this;
	this->IconParam::IconParam(srcobj);
	return *this;
}


// *********************************************
// EventData struct
// *********************************************


EventData::EventData(IEventInvoke* client, FunctionSwitch fswitch, long fid, IUPnPDevice* idev)
: _findid(fid)
, _idev(idev)
, _client(client)
, _switch(fswitch)
{}

EventData::EventData(IEventInvoke* client, FunctionSwitch fswitch, long fid, const wstring& devudn)
: _findid(fid)
, _devudn(devudn)
, _client(client)
, _switch(fswitch)
{}

EventData::EventData(IEventInvoke* client, FunctionSwitch fswitch, long fid)
: _findid(fid)
, _client(client)
, _switch(fswitch)
{}

EventData::EventData(IEventInvoke* client, FunctionSwitch fswitch, const Service* srv, const wstring& varname, const wstring& varvalue)
: _srv(srv)
, _varname(varname)
, _varvalue(varvalue)
, _client(client)
, _switch(fswitch)
{}

EventData::EventData(IEventInvoke* client, FunctionSwitch fswitch, const Service* srv)
: _srv(srv)
, _client(client)
, _switch(fswitch)
{}


// *********************************************
// EventInvokeProc thread routine
// used in upnp event handlers
// *********************************************


void EventInvokeProc(void* param)
{
	EventData* data = (EventData*)param;
	data->_client->InvokeClientEvent(data);
}


// *********************************************
// SrvEventCallback class
// *********************************************


SrvEventCallback::SrvEventCallback(Service* host)
: _refcount(0)
, _client(0)
, _host(host)
{
}

HRESULT SrvEventCallback::QueryInterface(const IID& riid, void** ppvObject)
{
	HRESULT result = ppvObject == 0 ? E_POINTER : S_OK;

	if(result == S_OK)
	{
		if(riid == IID_IUnknown || riid == IID_IUPnPServiceCallback)
		{
			*ppvObject = static_cast<IUPnPServiceCallback*>(this);
			this->AddRef();
		}
		else
		{
			*ppvObject = 0;
			result = E_NOINTERFACE;
		}
	}

	return result;
}

unsigned long SrvEventCallback::AddRef()
{
	return ::InterlockedIncrement(&_refcount);
}

unsigned long SrvEventCallback::Release()
{
	if(::InterlockedDecrement(&_refcount) == 0L)
	{
		delete this;
		return 0; // object deleted !!!
	}
	
	return _refcount;
}

void SrvEventCallback::InvokeClientEvent(const EventData* data)
{
	switch(data->_switch)
	{
	case EventData::StateVariableChanged :
		_client->ServiceEventVariableChanged(data->_srv, data->_varname, data->_varvalue);
		break;
	case EventData::ServiceInstanceDied :
		_client->ServiceEventInstanceDied(data->_srv);
		break;
	}
				
	delete data;
}

HRESULT SrvEventCallback::StateVariableChanged(IUPnPService* isrv, LPCWSTR varname, VARIANT varvalue)
{
	if(_client != 0)
	{
		wstring value;
		VARIANT vval;
		VariantInit(&vval);

		HRESULT hr = VariantCopy(&vval, &varvalue);
		if(hr == S_OK)
		{
			if(vval.vt != VT_BSTR)
				hr = VariantChangeType(&vval, &vval, VARIANT_ALPHABOOL | VARIANT_NOUSEROVERRIDE, VT_BSTR);

			if(hr == S_OK)
				value = V_BSTR(&vval);
		}

		VariantClear(&vval);

#ifdef UCPL_MULTITHREADED
		EventData* data = new EventData(this, EventData::StateVariableChanged, _host, varname, value);
		_beginthread(EventInvokeProc, 0, (void*)data);
#else
		_client->ServiceEventVariableChanged(_host, varname, value);
#endif
	}
	
	return S_OK; // The application should return S_OK
}

HRESULT SrvEventCallback::ServiceInstanceDied(IUPnPService* isrv)
{
	if(_client != 0)
	{
#ifdef UCPL_MULTITHREADED
		EventData* data = new EventData(this, EventData::ServiceInstanceDied, _host);
		_beginthread(EventInvokeProc, 0, (void*)data);
#else
		_client->ServiceEventInstanceDied(_host);
#endif
	}
	
	return S_OK; // The application should return S_OK
}

void SrvEventCallback::SetClientPtr(IServiceCallbackClient* client)
{
	_client = client;
}



// *********************************************
// DevFinderCallback class
// *********************************************


DevFinderCallback::DevFinderCallback(IFinderCallbackClient* client)
: _client(client)
, _refcount(0)
{
}

HRESULT DevFinderCallback::QueryInterface(const IID& riid, void** ppvObject)
{
	HRESULT result = ppvObject == 0 ? E_POINTER : S_OK;

	if(result == S_OK)
	{
		if(riid == IID_IUnknown || riid == IID_IUPnPDeviceFinderCallback)
		{
			*ppvObject = static_cast<IUPnPDeviceFinderCallback*>(this);
			this->AddRef();
		}
		else
		{
			*ppvObject = 0;
			result = E_NOINTERFACE;
		}
	}

	return result;
}

unsigned long DevFinderCallback::AddRef()
{
	return ::InterlockedIncrement(&_refcount);
}

unsigned long DevFinderCallback::Release()
{
	if(::InterlockedDecrement(&_refcount) == 0L)
	{
		delete this;
		return 0; // object deleted !!!
	}
	
	return _refcount;
}

void DevFinderCallback::InvokeClientEvent(const EventData* data)
{
	switch(data->_switch)
	{
	case EventData::DeviceAdded :
		{
			_client->Lock();

			try
			{
				_client->DeviceAdded(data->_findid, data->_idev);
				data->_idev->Release();
			}
			catch(std::exception)
			{
				data->_idev->Release();
				delete data;
				_client->UnLock();
				throw;
			}

			_client->UnLock();
		}
		break;
	case EventData::DeviceRemoved :
		{
			_client->Lock();

			try
			{
				_client->DeviceRemoved(data->_findid, data->_devudn);
			}
			catch(std::exception)
			{
				delete data;
				_client->UnLock();
				throw;
			}

			_client->UnLock();
		}
		break;
	case EventData::SearchComplete :
		_client->SearchComplete(data->_findid);
		break;
	}
				
	delete data;
}

HRESULT DevFinderCallback::DeviceAdded(long findid, IUPnPDevice* idev)
{
#ifdef UCPL_MULTITHREADED
	idev->AddRef();
	EventData* data = new EventData(this, EventData::DeviceAdded, findid, idev);
	_beginthread(EventInvokeProc, 0, (void*)data);
#else
		_client->DeviceAdded(findid, idev);
#endif
	return S_OK; // any value returned is ignored by Universal Plug and Play
}

HRESULT DevFinderCallback::DeviceRemoved(long findid, BSTR devudn)
{
#ifdef UCPL_MULTITHREADED
	EventData* data = new EventData(this, EventData::DeviceRemoved, findid, devudn);
	_beginthread(EventInvokeProc, 0, (void*)data);
#else
	_client->DeviceRemoved(findid, devudn);
#endif
	return S_OK; // any value returned is ignored by Universal Plug and Play
}

HRESULT DevFinderCallback::SearchComplete(long findid)
{
#ifdef UCPL_MULTITHREADED
	EventData* data = new EventData(this, EventData::SearchComplete, findid);
	_beginthread(EventInvokeProc, 0, (void*)data);
#else
	_client->SearchComplete(findid);
#endif
	return S_OK; // any value returned is ignored by Universal Plug and Play
}



// *********************************************
// DocAccessData struct
// *********************************************


DocAccessData::DocAccessData()
{
	memset(&_addr, 0, sizeof(_addr));
}

DocAccessData::DocAccessData(const wstring &url)
{
	memset(&_addr, 0, sizeof(_addr));

	if(!SetData(url))
		throw invalid_argument("invalid url or setting data failed");
}

bool DocAccessData::SetData(const wstring& url)
{
	bool result = false;

	if(!url.empty())
	{
		_url = url;

		if(SetAddress())
		{
			if(LoadData())
			{
				if(GetXmlDataRoot(L"urlbase", _urlbase))
					// inside doc, urlbase tag was not empty
					result = true;
				else
					// urlbase tag was empty, so, try to extract it from DocAccessData::_url
					result = SetBaseURL();
			}
		}
	}

	return result;
}

bool DocAccessData::SetAddress()
{
	if(_url.empty())
		return false;

	wstring srcaddr(_url);
	wstring path;
	sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	int bufflen = sizeof(addr);
	wstring::size_type diff = 0;
	int err = 0;

	if(srcaddr.find('/') != wstring::npos)
	{
		if(srcaddr.find(L"http://") != wstring::npos)
			srcaddr = srcaddr.substr(7);

		if((diff = srcaddr.find('/')) > 0)
		{
			path = srcaddr.substr(diff);

			srcaddr = srcaddr.substr(0, diff);
		}
	}

	if(WSAStringToAddressW(const_cast<wchar_t*>(srcaddr.c_str()), AF_INET, 0, (sockaddr*)&addr, &bufflen) == SOCKET_ERROR)
	{
		if((err = WSAGetLastError()) != WSAEFAULT)
			return false;
		else
		{
			if(WSAStringToAddressW(const_cast<wchar_t*>(srcaddr.c_str()), AF_INET, 0, (sockaddr*)&addr, &bufflen) == SOCKET_ERROR)
				return false;
		}
	}
		
	_addr.sin_addr = addr.sin_addr;
	_addr.sin_family = addr.sin_family;
	_addr.sin_port = addr.sin_port;

	_path = path;

	return true;
}

bool DocAccessData::SetBaseURL()
{
	wstring::size_type diff = 0;
	wstring::size_type pos = 0;
	wstring::size_type charstocopy = 0;
	wstring result;

	if(_url.empty())
		return false;

	if(_url.find(L"http://") != wstring::npos)
		pos = 7; // store index of "http://" end

	// if diff = pos, then string begins with "/" -> error
	if((diff = _url.find('/', pos)) != pos)
	{
		if(diff > pos) 
			// after "http://" is more (or is only) "/", so cut path
			charstocopy = diff;
		else if(diff == wstring::npos)
			// if diff = -1, then after "http://" or at all there was not "/"
			// copy entire string
			charstocopy = _url.size();
	}
	// else error, don't copy

	result = _url.substr(0, charstocopy);
	if(!result.empty())
	{
		_urlbase = result;
		return true;
	}

	return false;
}

bool DocAccessData::GetXmlDataRoot(const wstring& tag, /*out*/wstring& value) const
{
	bool result = false;

	mstring _xdoc(_doc.begin(), _doc.end());
	mstring _tag(tag.begin(), tag.end());
	mstring _value;

	CMarkup doc;
	doc.SetDoc(_xdoc);
	doc.SetDocFlags(CMarkup::MDF_IGNORECASE);
	doc.ResetPos();
	doc.FindElem();
	doc.IntoElem();
	if(doc.FindElem(_tag))
	{
		if(!(_value = doc.GetData()).empty())
			result = !value.assign(_value.begin(), _value.end()).empty();
	}

	return result;
}

bool DocAccessData::GetXmlDataScpdUrl(const wstring& srvid, /*out*/wstring& scpdurl) const
{
	mstring _xdoc(_doc.begin(), _doc.end());
	mstring _tag(srvid.begin(), srvid.end());
	mstring _value;

	int k = 0;

	CMarkup doc;
	doc.SetDoc(_xdoc);
	doc.SetDocFlags(CMarkup::MDF_IGNORECASE);
	doc.ResetPos();
	doc.FindElem();
	doc.IntoElem();

	while(1)
	{
		if(doc.FindElem(_T("device")))
		{
			doc.IntoElem();
			k++;

			// find service
			doc.SavePos();
			if(doc.FindElem(_T("servicelist")))
			{
				doc.IntoElem();
				while(doc.FindElem(_T("service")))
				{
					if(doc.FindChildElem(_T("serviceid")))
					{
						if(_tag.compare(doc.GetChildData()) == 0)
						{
							doc.ResetChildPos();
							if(doc.FindChildElem(_T("scpdurl")))
							{
								_value = doc.GetChildData();
								return !scpdurl.assign(_value.begin(), _value.end()).empty();
							}
						}
					}
				}
				doc.OutOfElem();
			}
			doc.RestorePos();
			// end of finding service

			if(!doc.FindElem(_T("devicelist")))
				doc.OutOfElem();
			else
				doc.IntoElem();
		}
		else
		{
			doc.OutOfElem();
			k--;
		}

		if(k < 1) break;
	}

	return false;
}

bool DocAccessData::GetXmlDataVariables(/*out*/InfoData& varsinfo) const
{
	mstring _xdoc(_doc.begin(), _doc.end());
	mstring _value;

	bool result = false;
	varsinfo.clear();

	CMarkup doc;
	doc.SetDoc(_xdoc);
	doc.SetDocFlags(CMarkup::MDF_IGNORECASE);
	doc.ResetPos();
	doc.FindElem();
	doc.IntoElem();

	if(doc.FindElem(_T("servicestatetable")))
	{
		int varcount = 0;

		doc.IntoElem();

		while(doc.FindElem(_T("statevariable")))
		{
			if(doc.FindChildElem(_T("name")))
			{
				InfoDataItem item;

				_value = doc.GetChildData();
				item.first = wstring(_value.begin(), _value.end());
				_value = doc.GetAttrib(_T("sendevents"));
				item.second = wstring(_value.begin(), _value.end());

				if(!item.first.empty() && !item.second.empty())
					varsinfo.insert(item);
			}

			++varcount;
		}

		if(varcount != 0 && varcount == varsinfo.size())
			result = true;
	}

	return result;
}

bool DocAccessData::GetXmlDataActions(/*out*/StrList& actlist) const
{
	mstring _xdoc(_doc.begin(), _doc.end());
	mstring _value;

	bool result = true;
	actlist.clear();

	CMarkup doc;
	doc.SetDoc(_xdoc);
	doc.SetDocFlags(CMarkup::MDF_IGNORECASE);
	doc.ResetPos();
	doc.FindElem();
	doc.IntoElem();

	if(doc.FindElem(_T("actionlist")))
	{
		int actcount = 0;

		doc.IntoElem();

		while(doc.FindElem(_T("action")))
		{
			if(doc.FindChildElem(_T("name")))
			{
				if(!(_value = doc.GetChildData()).empty())
					actlist.push_back(wstring(_value.begin(), _value.end()));
			}

			++actcount;
		}

		if(actcount == 0 || actcount != actlist.size())
			result = false;
	}

	return result;
}

bool DocAccessData::GetXmlDataActionArgs(const wstring& aname, /*out*/InfoDataList& inflist) const
{
	//TCHAR* cnames[] = {_T("Arg name"), _T("Direction"), _T("Var name"), _T("Type"), _T("Events"), _T("Allowed values"), _T("Min"), _T("Max"), _T("Step"), _T("Default")};

	bool result = false;
	inflist.clear();

	mstring _xdoc(_doc.begin(), _doc.end());
	mstring _aname(aname.begin(), aname.end());
	mstring _value;

	CMarkup doc;
	doc.SetDoc(_xdoc);
	doc.SetDocFlags(CMarkup::MDF_IGNORECASE);
	doc.ResetPos();
	doc.FindElem();
	doc.IntoElem();

	if(doc.FindElem(_T("actionlist")))
	{
		doc.IntoElem();

		while(doc.FindElem(_T("action")))
		{
			if(doc.FindChildElem(_T("name")))
			{
				_value = doc.GetChildData();
				if(_value.compare(_aname) == 0)
				{
					result = true; // action has been found, but it may not have arguments 

					doc.ResetChildPos();
					doc.IntoElem();
					if(doc.FindElem(_T("argumentlist")))
					{
						int argcount = 0;

						doc.IntoElem();

						while(doc.FindElem(_T("argument"))) // successive elements of list
						{
							bool localresult = false;
							InfoData::size_type localcount = 0;

							// get info about argument
							InfoData idata;

							if(doc.FindChildElem(_T("name")))
							{
								if(!(_value = doc.GetChildData()).empty())
									idata.insert(InfoDataItem(L"Arg name", wstring(_value.begin(), _value.end())));
							}
							++localcount;
							
							doc.ResetChildPos();
							if(doc.FindChildElem(_T("direction")))
							{
								if(!(_value = doc.GetChildData()).empty())
									idata.insert(InfoDataItem(L"Direction", wstring(_value.begin(), _value.end())));
							}
							++localcount;
							
							doc.ResetChildPos();
							if(doc.FindChildElem(_T("relatedstatevariable")))
							{
								if(!(_value = doc.GetChildData()).empty())
									idata.insert(InfoDataItem(L"Var name", wstring(_value.begin(), _value.end())));

								// get info about related variable
								doc.SavePos(_T("argument"));
								localresult = GetXmlDataActionVarInfo(wstring(_value.begin(), _value.end()), idata);
								doc.RestorePos(_T("argument"));
							}
							++localcount;

							if(localcount <= idata.size() && localresult)
								inflist.push_back(idata); // add argument to list, without delete
								
							++argcount;
						}

						// check loop counter
						if(argcount == 0 || argcount != inflist.size())
							result = false;
					}
						
					break;
				}
			}
		}
	}

	return result;
}

bool DocAccessData::GetXmlDataActionArgsCount(const wstring& aname, /*out*/int& argcount) const
{
	bool result = false;
	int i = 0;

	mstring _xdoc(_doc.begin(), _doc.end());
	mstring _aname(aname.begin(), aname.end());
	mstring _value;

	CMarkup doc;
	doc.SetDoc(_xdoc);
	doc.SetDocFlags(CMarkup::MDF_IGNORECASE);
	doc.ResetPos();
	doc.FindElem();
	doc.IntoElem();

	if(doc.FindElem(_T("actionlist")))
	{
		doc.IntoElem();

		while(doc.FindElem(_T("action")))
		{
			if(doc.FindChildElem(_T("name")))
			{
				_value = doc.GetChildData();
				if(_value.compare(_aname) == 0)
				{
					result = true; // action may not have arguments

					doc.ResetChildPos();
					doc.IntoElem();
					if(doc.FindElem(_T("argumentlist")))
					{
						doc.IntoElem();
						while(doc.FindElem(_T("argument"))) // successive elements of list
							++i;

						if(i == 0)
							result = false;
					}
						
					break;
				}
			}
		}
	}

	argcount = i;

	return result;
}

bool DocAccessData::GetXmlDataActionVarInfo(const wstring& vname, /*in/out*/InfoData& infdata) const
{
	//TCHAR* cnames[] = {_T("Arg name"), _T("Direction"), _T("Var name"), _T("Type"), _T("Events"), _T("Allowed values"), _T("Min"), _T("Max"), _T("Step"), _T("Default")};
	
	bool result = false;

	mstring _xdoc(_doc.begin(), _doc.end());
	mstring _vname(vname.begin(), vname.end());
	mstring _value;

	CMarkup doc;
	doc.SetDoc(_xdoc);
	doc.SetDocFlags(CMarkup::MDF_IGNORECASE);
	doc.ResetPos();
	doc.FindElem();
	doc.IntoElem();

	if(doc.FindElem(_T("servicestatetable")))
	{
		doc.IntoElem();

		while(doc.FindElem(_T("statevariable")))
		{
			if(doc.FindChildElem(_T("name")))
			{
				_value = doc.GetChildData();
				if(_value.compare(_vname) == 0) // successive elements of list
				{
					int inscount = 0;
					int inicount = infdata.size();
					
					if(!(_value = doc.GetAttrib(_T("sendevents"))).empty())
						infdata.insert(InfoDataItem(L"Events", wstring(_value.begin(), _value.end())));
					++inscount;

					doc.ResetChildPos();
					doc.IntoElem();

					if(doc.FindElem(_T("datatype")))
					{
						if(!(_value = doc.GetData()).empty())
							infdata.insert(InfoDataItem(L"Type", wstring(_value.begin(), _value.end())));
					}
					++inscount;

					doc.ResetMainPos();
					if(doc.FindElem(_T("defaultvalue")))
					{
						if(!(_value = doc.GetData()).empty())
						{
							infdata.insert(InfoDataItem(L"Default", wstring(_value.begin(), _value.end())));
							++inscount; // incompatible with standard
						}
					}

					doc.ResetMainPos();
					if(doc.FindElem(_T("allowedvaluerange")))
					{
						if(doc.FindChildElem(_T("minimum")))
						{
							if(!(_value = doc.GetChildData()).empty())
							{
								infdata.insert(InfoDataItem(L"Min", wstring(_value.begin(), _value.end())));
								++inscount; // incompatible with standard
							}
						}
							
						doc.ResetChildPos();
						if(doc.FindChildElem(_T("maximum")))
						{
							if(!(_value = doc.GetChildData()).empty())
							{
								infdata.insert(InfoDataItem(L"Max", wstring(_value.begin(), _value.end())));
								++inscount; // incompatible with standard
							}
						}

						doc.ResetChildPos();
						if(doc.FindChildElem(_T("step")))
						{
							if(!(_value = doc.GetChildData()).empty())
							{
								infdata.insert(InfoDataItem(L"Step", wstring(_value.begin(), _value.end())));
								++inscount; // incompatible with standard
							}
						}
					}

					doc.ResetMainPos();
					if(doc.FindElem(_T("allowedvaluelist")))
					{
						_value.clear();
						while(doc.FindChildElem(_T("allowedvalue")))
							_value.append(doc.GetChildData()).append(_T("; "));
							
						if(!_value.empty())
						{
							infdata.insert(InfoDataItem(L"Allowed values", wstring(_value.begin(), _value.end())));
							++inscount; // incompatible with standard
						}
					}

					if(inscount == (infdata.size() - inicount))
						result = true;

					break;
				}
			}
		}
	}

	return result;
}

bool DocAccessData::GetXmlDataIconsList(/*out*/IconList& icolist) const
{
	bool result = false;
	icolist.clear();

	mstring _xdoc(_doc.begin(), _doc.end());
	mstring vmime, vwidth, vheight, vdepth, vurl;

	CMarkup doc;
	doc.SetDoc(_xdoc);
	doc.SetDocFlags(CMarkup::MDF_IGNORECASE);
	doc.ResetPos();
	doc.FindElem();
	doc.IntoElem();

	if(doc.FindElem(_T("device")))
	{
		result = true; // device may not have icons
		doc.IntoElem();

		if(doc.FindElem(_T("iconlist")))
		{
			int icocount = 0;

			doc.IntoElem();
			while(doc.FindElem(_T("icon")))
			{
				if(doc.FindChildElem(_T("mimetype")))
					vmime = doc.GetChildData();
				doc.ResetChildPos();
				if(doc.FindChildElem(_T("width")))
					vwidth = doc.GetChildData();
				doc.ResetChildPos();
				if(doc.FindChildElem(_T("height")))
					vheight = doc.GetChildData();
				doc.ResetChildPos();
				if(doc.FindChildElem(_T("depth")))
					vdepth = doc.GetChildData();
				doc.ResetChildPos();
				if(doc.FindChildElem(_T("url")))
					vurl = doc.GetChildData();

				if(!vurl.empty() && !vmime.empty() && !vwidth.empty() && !vheight.empty() && !vdepth.empty())
					// save icon's list
					icolist.push_back(IconParam(wstring(vurl.begin(), vurl.end()), wstring(vmime.begin(), vmime.end()), _ttoi(vwidth.c_str()), _ttoi(vheight.c_str()), _ttoi(vdepth.c_str())));
					
				vurl.clear(); vmime.clear(); vwidth.clear(); vheight.clear(); vdepth.clear();

				++icocount;
			}

			if(icocount == 0 || icocount != icolist.size())
				result = false;
		}
	}

	return result;
}

bool DocAccessData::LoadData(long mili_seconds/* = 100L*/)
{
	bool result = false;

	if(_addr.sin_addr.s_addr == 0 || _addr.sin_port == 0 || _path.empty())
		return result;

	// timeout for function select, default 100 ms
	static timeval _timeout = {0L, 100000L};
	_timeout.tv_usec = mili_seconds * 1000L;
	timeval* _ptimeout = _addr.sin_addr.s_addr == inet_addr("127.0.0.1") ? &_timeout : 0;

	const int rbsize = 4096;	// internal buffer size
	char rbuff[rbsize] = {0};	// internal temporary receive buffer
	int rbshift = 0;			// index in internal buffer of current begin of free space // index in buffer of begin of actual data to copy
	int err = 0;
	int b = 0;					// bytes curently received
	int tb = 0;					// bytes totally received
	string respbuff;
	string headertail("\r\n\r\n");


	std::ostringstream os;
	os << "GET " << string(_path.begin(), _path.end()) << " HTTP/1.1\r\nHost: " << inet_ntoa(_addr.sin_addr);
	u_short port = ntohs(_addr.sin_port);
	if(port > 0 && port < 65535)
		os << ':' << port;
	os << headertail;

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if(connect(s, (sockaddr*)&_addr, sizeof(sockaddr_in)) == SOCKET_ERROR)
		err = WSAGetLastError();
	
	if((b = send(s, os.str().c_str(), os.str().length(), 0)) == SOCKET_ERROR)
		err = WSAGetLastError();

	// switch client socket to non-blocking mode
	unsigned long argp = 1uL;
	if(ioctlsocket(s, FIONBIO, &argp) == SOCKET_ERROR)
		err = WSAGetLastError();

	b = SOCKET_ERROR;
	int sel; // result of select

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(s, &fds);

	while((sel = select(0, &fds, 0, 0, _ptimeout)) != SOCKET_ERROR)
	{
		if(sel == 0)
			break; // timeout
		else
			b = recv(s, rbuff + rbshift, rbsize - rbshift, 0);

		// finish loop if connection has been gracefully closed
		if(b == 0)
			break;

		// sum of all received chars
		tb += b;
		// sum of currently received chars
		rbshift += b;

		// temporary buffer has been filled
		// thus copy data to response buffer
		if(rbshift == rbsize)
		{
			respbuff.append(rbuff, rbshift);

			// reset current counter
			rbshift = 0;
		}
	}

	// if error occured during receiving of data
	if(b == SOCKET_ERROR)
		err = WSAGetLastError();

	// close connection gracefully
	shutdown(s, SD_SEND);
	while(int bc = recv(s, rbuff, rbsize, 0))
		if(bc == SOCKET_ERROR) break;
	closesocket(s);

	// analyse received data
	if(tb > 0)
	{
		// copy any remaining data to response buffer
		if(rbshift > 0)
			respbuff.append(rbuff, rbshift);

		// check response code in header
		if(respbuff.substr(0, respbuff.find("\r\n")).find("200 OK") != string::npos)
		{
			int cntlen_comp = 0;	// computed response's content length
			int cntlen_get = 0;		// retrieved response's content length
			string::size_type pos;
			string::size_type posdata = respbuff.find(headertail) + headertail.length();

			if(posdata != string::npos)
			{
				// compute content length
				cntlen_comp = tb - posdata;

				if(cntlen_comp > 0)
				{
					if(b == 0)
					{
						// connection has been gracefully closed
						// thus received data should be valid
						cntlen_get = cntlen_comp;
					}
					else
					{
						// get content length from http header
						// to check if number of received data is equal to number of sent data
						string header = respbuff.substr(0, posdata);
						transform(header.begin(), header.end(), header.begin(), tolower);
						if((pos = header.find("content-length:")) != string::npos)
							std::istringstream(header.substr(pos, header.find("\r\n", pos)).substr(15)) >> cntlen_get;
					}

					if(cntlen_comp == cntlen_get)
						result = !_doc.assign(respbuff.begin() + posdata, respbuff.end()).empty();
				}
			}
		}
	}
	
	return result;
}


// *********************************************
// Action class
// *********************************************


Action::Action(const Service* srv, const wstring& name)
: _parent(srv)
, _name(name)
, _argcount(0)
, _inargcount(0)
, _complete(false)
, _inargscomplete(false)
{
	if(_parent == 0 || _name.empty())
		throw invalid_argument("invalid parent's pointer or empty name");
}

wstring Action::GetName() const
{
	return _name;
}

const Service& Action::GetParentService() const
{
	return *_parent;
}

int Action::GetArgsCount()
{
	if(!_complete)
		_complete = SetArgsCount();

	return _complete ? _argcount : -1;
}
	
int Action::GetInArgsCount()
{
	if(!_inargscomplete)
		_inargscomplete = InitInArgsList();

	return _inargscomplete ? _inargcount : -1;
}

bool Action::GetInfo(/*out*/InfoDataList& inflist) const
{
	return _parent->GetAccessData()->GetXmlDataActionArgs(_name, inflist);
}

bool Action::SetInArgs(const StrList& args)
{
	bool result = false;

	if(!_inargscomplete)
		_inargscomplete = InitInArgsList();

	if(_inargscomplete && _in.size() <= args.size())
	{
		StrIterator si = args.begin();
		vector<InfoDataItem>::iterator ai;
		for(ai = _in.begin(); ai != _in.end(); ++ai, ++si)
			(*ai).second = *si;

		result = (ai == _in.end());
	}

	return result;
}

bool Action::SetInArgs(const wstring& arg, unsigned int index)
{
	bool result = false;

	if(!_inargscomplete)
		_inargscomplete = InitInArgsList();

	if(_inargscomplete && index >= 0 && index < _in.size())
	{
		_in[index].second = arg;
		result = true;
	}

	return result;
}

bool Action::GetInArgs(ArgsArray& args)
{
	bool result = false;

	if(!_inargscomplete)
		_inargscomplete = InitInArgsList();

	if(_inargscomplete)
	{
		args = _in;

		result = (args.size() == _in.size());
	}

	return result;
}

bool Action::GetInArgs(InfoDataItem& arg, unsigned int index)
{
	bool result = false;

	if(!_inargscomplete)
		_inargscomplete = InitInArgsList();

	if(_inargscomplete && index >= 0 && index < _in.size())
	{
		arg = _in[index];
		result = true;
	}

	return result;
}

int Action::Invoke(ArgsArray& argsout) const
{
	// action name
	BSTR aname = SysAllocString(_name.c_str());
	if(aname == 0)
		return -1;

	// interface of UPnP service
	IUPnPService* _iservice = 0;
	_parent->GetInterface(&_iservice);
	if(_iservice == 0)
	{
		SysFreeString(aname);
		return -1;
	}


	HRESULT hr = S_OK;
	long inargscount = 0;			// number of input arguments
	long outargscount = 0;			// number of output arguments

	VARIANT	inargs;					// Invoke argument in
	VARIANT outargs;				// Invoke argument out
	VARIANT retval;					// Invoke argument ret
	VARIANT inval;					// temp input value
	SAFEARRAY* arr_inargs = 0;		// array for input arguments
	VARIANT outval;					// temp output value
	SAFEARRAY* arr_outargs = 0;		// array for output arguments
	
	long arr_index[1];				// current index of safe array
	

	// get number of arguments
	inargscount = _inargcount;//argsin.size();

	// initialize variants
	VariantInit(&inargs);
	VariantInit(&outargs);
	VariantInit(&retval);

	// create input array
	SAFEARRAYBOUND bounds[1];
	bounds[0].lLbound = 0;
	bounds[0].cElements = inargscount;
	arr_inargs = SafeArrayCreate(VT_VARIANT, 1, bounds);

	if(arr_inargs != 0)
	{
		// fill input safe array
		ArgsIterator argi = _in.begin();
		for(long i = 0; i < inargscount; ++i, ++argi)
		{
			arr_index[0] = i;

			VariantInit(&inval);
			inval.vt = VT_BSTR;
			V_BSTR(&inval) = SysAllocString((*argi).second.c_str()); // value of arg

			if(V_BSTR(&inval) != 0)
			{
				hr = S_OK;

				VARTYPE vt = GetVariantType((*argi).first); // type of arg
				if(vt != VT_BSTR)
					// change type according to props list
					hr = VariantChangeType(&inval, &inval, VARIANT_NOUSEROVERRIDE, vt);

				if(hr == S_OK)
					SafeArrayPutElement(arr_inargs, arr_index, (void*)&inval);
			}

			VariantClear(&inval);
		}

		// assign input array to input argument
		inargs.vt = VT_ARRAY | VT_VARIANT;
		V_ARRAY(&inargs) = arr_inargs;

		// invoke action
		hr = _iservice->InvokeAction(aname, inargs, &outargs, &retval);

		if(hr == S_OK) // invoke succeeded
		{
			// data from outargs
			arr_outargs = V_ARRAY(&outargs);

			long arrsize = 0;
			VARTYPE vartype;

			if(SafeArrayGetUBound(arr_outargs, 1, &arrsize) == S_OK)
			{
				for(long i = 0; i <= arrsize; ++i)
				{
					arr_index[0] = i;

					VariantInit(&outval);

					hr = SafeArrayGetElement(arr_outargs, arr_index, (void*)&outval);
					if(hr == S_OK)
					{
						vartype = outval.vt;

						if(outval.vt != VT_BSTR)
							hr = VariantChangeType(&outval, &outval, VARIANT_ALPHABOOL | VARIANT_NOUSEROVERRIDE, VT_BSTR);

						if(hr == S_OK)
						{
							argsout.push_back(InfoDataItem(GetTypeDescr(vartype), V_BSTR(&outval)));
							++outargscount;
						}
					}

					VariantClear(&outval);
				}
			}

			// data from retval
			if(retval.vt != VT_EMPTY)
			{
				hr = S_OK;
				vartype = retval.vt;

				if(retval.vt != VT_BSTR)
					hr = VariantChangeType(&retval, &retval, VARIANT_ALPHABOOL | VARIANT_NOUSEROVERRIDE, VT_BSTR);

				if(hr == S_OK)
				{
					argsout.push_back(InfoDataItem(GetTypeDescr(vartype), wstring(L"Return value: ").append(V_BSTR(&retval))));
					outargscount = outargscount > 0 ? ++outargscount : -2;
				}
			}
		}
		else // invoke failed
		{
			// return only errors from InvokeAction
			std::wostringstream wos;
			wos << L"ERROR: 0x" << std::hex << hr;
			argsout.push_back(InfoDataItem(L"string", wos.str()));
			argsout.push_back(InfoDataItem(L"string", GetErrorMessage(hr)));

			// if error occurred then return value can contains description of error
			if(retval.vt == VT_BSTR)
				argsout.push_back(InfoDataItem(L"string", V_BSTR(&retval)));

			outargscount = -1; // error
		}

		// destroy input array
		SafeArrayDestroy(arr_inargs);
		// array has been destroyed thus set related variant to empty
		// to avoid destroing array again during clear of variant
		inargs.vt = VT_EMPTY;
	}

	// clear variants
	VariantClear(&inargs);
	VariantClear(&outargs);
	VariantClear(&retval);

	SysFreeString(aname);
	_iservice->Release();

	return outargscount;
}

bool Action::SetArgsCount()
{
	bool result = false;
	int argcount = 0;

	if(result = _parent->GetAccessData()->GetXmlDataActionArgsCount(_name, argcount))
		_argcount = argcount;

	return result;
}

bool Action::InitInArgsList()
{
	bool result = false;

	InfoDataList idl;
	if(GetInfo(idl))
	{
		// by the way, number of arguments
		if(!_complete)
		{
			_argcount = idl.size();
			_complete = true;
		}

		ArgsArray::size_type incount = 0;
		_in.clear();

		//
		for(InfoDataList::iterator ii = idl.begin(); ii != idl.end(); ++ii)
		{
			if((*ii)[L"Direction"] == L"in")
			{
				_in.push_back(InfoDataItem((*ii)[L"Type"], L""));
				++incount;
			}
		}

		if(incount == _in.size())
		{
			_inargcount = incount;
			result = true;
		}
	}

	return result;
}



// *********************************************
// Service class
// *********************************************


Service::Service(IUPnPService* isrv, const Device& parentdev)
: _iservice(isrv)
, _parent(parentdev)
, _isrvcback(0)
{
	if(_iservice == 0)
		throw invalid_argument("null IUPnPService pointer");
	else
		_iservice->AddRef();

	if(!SetServiceID())
		throw invalid_argument("retrieving of service id failed");

	if(!SetAccessData())
		throw invalid_argument("retrieving of access data failed");

	if(!EnumActions())
		throw invalid_argument("retrieving of action's names failed");

	// create callback
	// callback object is deleting in dtor in SrvEventCallback::Release()
	_isrvcback = new SrvEventCallback(this);
	_isrvcback->AddRef();
}

Service::~Service()
{
	// release callback
	_isrvcback->Release();

	// release IUPnPService
	_iservice->Release();
}

bool Service::SetAccessData()
{
	bool result = false;

	const DocAccessData* devdad = _parent.GetAccessData();

	_accessdata._doc = devdad->_doc;
	
	// get relative scpd url
	if(_accessdata.GetXmlDataScpdUrl(_name, _accessdata._path))
	{
		_accessdata._url = devdad->_url;

		if(_accessdata.SetBaseURL())
		{
			// combine base url & scpdurl path to scpd uri
			wstring missingslash;
			if(*(--_accessdata._urlbase.end()) != '/' && *(_accessdata._path.begin()) != '/')
				missingslash = L"/";
			// save scpd uri in service object
			_accessdata._url.assign(_accessdata._urlbase).append(missingslash).append(_accessdata._path);

			// got scpd uri, so load scpd descr document

			// get necessary address info and load scpd's content
			if(_accessdata.SetAddress() && _accessdata.LoadData())
				result = true;
		}
	}

	return result;
}

int Service::GetActionCount() const
{
	return _actions.size();
}

ActionIterator Service::GetCollectionBegin() const
{
	return _actions.begin();
}

ActionIterator Service::GetCollectionEnd() const
{
	return _actions.end();
}

const Action& Service::GetAction(unsigned int index) const
{
	ActionIterator ai;

	if(!_actions.empty() && index >= 0 && index < _actions.size())
		for(ai = _actions.begin(); index > 0; ++ai, --index) ;
	else
		throw invalid_argument("index out of range");

	return *ai;
}

const Action& Service::GetAction(const wstring& aname) const
{
	bool found = false;
	ActionIterator ai;

	for(ai = _actions.begin(); ai != _actions.end(); ++ai)
		if((*ai).GetName().compare(aname) == 0)
		{
			found = true;
			break;
		};

	if(!found)
		throw invalid_argument("not found");

	return *ai;
}

wstring Service::GetServiceID() const
{
	return _name;
}

wstring Service::GetServiceTypeID() const
{
	return _typeid;
}

long Service::GetLastTransportStatus() const
{
	long tstatus = 0;
	_iservice->get_LastTransportStatus(&tstatus);
	return tstatus;
}

const Device& Service::GetParentDevice() const
{
	return _parent;
}

// adds callback for events and sets its client
bool Service::SetCallbackClient(IServiceCallbackClient* iclient)
{
	bool result = false;

	if(iclient != 0)
	{
		_isrvcback->SetClientPtr(iclient);
		result = _iservice->AddCallback(_isrvcback) == S_OK;
	}

	return result;
}

// with AddRef, don't forget to release interface when unused
void Service::GetInterface(IUPnPService** isrv) const
{
	if(_iservice != 0)
	{
		*isrv = _iservice;
		(*isrv)->AddRef();
	}
}

wstring Service::GetScpdURL() const
{
	return _accessdata._url;
}

wstring Service::GetScpdContent() const
{
	return _accessdata._doc;
}

const DocAccessData* Service::GetAccessData() const
{
	return &_accessdata;
}

bool Service::SetServiceID()
{
	BSTR btmp = 0;

	_iservice->get_Id(&btmp);
	if(btmp != 0)
	{
		_name.assign(btmp);
		SysFreeString(btmp);
		btmp = 0;
	}
	
	_iservice->get_ServiceTypeIdentifier(&btmp);
	if(btmp != 0)
	{
		_typeid.assign(btmp);
		SysFreeString(btmp);
		btmp = 0;
	}

	return !_name.empty() && !_typeid.empty();
}

bool Service::EnumActions()
{
	bool result = false;

	StrList actions;
	if(_accessdata.GetXmlDataActions(actions))
	{
		StrIterator si;
		for(si = actions.begin(); si != actions.end(); ++si)
			_actions.push_back(Action(this, *si));

		result = (_actions.size() == actions.size());
	}

	return result;
}

bool Service::GetServiceInfo(InfoData& data) const
{
	InfoData::size_type inscount = 0;

	data.insert(InfoDataItem(L"Service ID", _name));
	++inscount;
	
	data.insert(InfoDataItem(L"Service type", _typeid));
	++inscount;

	data.insert(InfoDataItem(L"SCPD URL", _accessdata._url));
	++inscount;

	long tstatus = 0;
	wstring statstr;
	_iservice->get_LastTransportStatus(&tstatus);
	if(tstatus > 0)
	{
		std::wostringstream wos;
		wos << tstatus;
		statstr = wos.str();
	}
	data.insert(InfoDataItem(L"Last status", statstr));
	++inscount;

	return inscount == data.size();
}

bool Service::GetServiceVariables(VarData& data) const
{
	bool result = false;

	InfoData idata;
	if(_accessdata.GetXmlDataVariables(idata))
	{
		data.clear();
		for(InfoIterator ii = idata.begin(); ii != idata.end(); ++ii)
			data.insert(VarDataItem((*ii).first, (*ii).second == L"yes"));
		
		result = (idata.size() == data.size());
	}

	return result;
}



// *********************************************
// Device class
// *********************************************


Device::Device(IUPnPDevice* idev, const Device* parentdev/* = 0*/)
: _idevice(idev)
, _parent(parentdev)
{
	if(_idevice == 0)
		throw invalid_argument("null IUPnPDevice pointer");
	else
		_idevice->AddRef();

	// for root device retrieve access data
	if(_parent == 0 && !_accessdata.SetData(GetDocURL()))
		throw invalid_argument("retrieving of access data failed");

	if(!SetUDN())
		throw invalid_argument("retrieving of UDN failed");

	if(!SetType())
		throw invalid_argument("retrieving of type failed");

	GenerateFriendlyName();

	// enumerate member devices and services
	if(!EnumDev())
		throw invalid_argument("collecting of member devices failed");

	// retrieve list of icons from descr document
	if(_parent == 0)
		_accessdata.GetXmlDataIconsList(_icons);
}

Device::~Device()
{
	// clean up lists of services and devices
	RemoveAllDevices();
	RemoveAllServices();

	// release IUPnPDevice
	if(_idevice != 0)
		_idevice->Release();
}

wstring Device::GetUDN() const
{
	return _udn;
}

wstring Device::GetFriendlyName() const
{
	return _name;
}

wstring Device::GetType() const
{
	return _type;
}

void Device::AddService(IUPnPService* isrv)
{
	_services.push_back(new Service(isrv, *this));
}

int Device::GetServiceListCount() const
{
	return _services.size();
}

ServiceIterator Device::GetServiceListBegin() const
{
	return _services.begin();
}

ServiceIterator Device::GetServiceListEnd() const
{
	return _services.end();
}

const Service* Device::GetService(unsigned int index) const
{
	if(index < 0 || index >= _services.size())
		throw invalid_argument("index out of range");

	ServiceIterator si = _services.begin();
	for(; index > 0; ++si, --index) ;

	return *si;
}

void Device::AddDevice(IUPnPDevice* idev)
{
	_devices.push_back(new Device(idev, this));
}

int Device::GetDeviceListCount() const
{
	return _devices.size();
}

DeviceIterator Device::GetDeviceListBegin() const
{
	return _devices.begin();
}

DeviceIterator Device::GetDeviceListEnd() const
{
	return _devices.end();
}

const Device* Device::GetDevice(unsigned int index) const
{
	if(index < 0 || index >= _devices.size())
		throw invalid_argument("index out of range");

	DeviceIterator di = _devices.begin();
	for(; index > 0; ++di, --index) ;

	return *di;
}

bool Device::IsDeviceListEmpty() const
{
	return _devices.empty();
}

int Device::GetIconsListCount() const
{
	return _icons.size();
}

IconIterator Device::GetIconsListBegin() const
{
	return _icons.begin();
}

const IconParam& Device::GetIcon(unsigned int index) const
{
	if(index < 0 || index >= _icons.size())
		throw invalid_argument("index out of range");

	IconIterator ii = _icons.begin();
	for(; index > 0; ++ii, --index) ;

	return *ii;
}

bool Device::IsIconsListEmpty() const
{
	return _icons.empty();
}

IconIterator Device::GetIconsListEnd() const
{
	return _icons.end();
}

const Device* Device::GetParentDevice() const
{
	return _parent;
}

const Device* Device::GetRootDevice() const
{
	const Device* thisroot = this;

	while(thisroot->GetParentDevice() != 0)
		thisroot = thisroot->GetParentDevice();

	return thisroot;
}

bool Device::IsRoot() const
{
	return (_parent == 0);
}

void Device::RemoveAllDevices()
{
	if(!_devices.empty())
	{
		for(list<Device*>::iterator di = _devices.begin(); di != _devices.end(); ++di)
		{
			delete *di;
			*di = 0;
		}
		
		_devices.clear();
	}
}

void Device::RemoveAllServices()
{
	if(!_services.empty())
	{
		for(list<Service*>::iterator si = _services.begin(); si != _services.end(); ++si)
		{
			delete *si;
			*si = 0;
		}

		_services.clear();
	}
}

void Device::GetInterface(IUPnPDevice** idev) const
{
	*idev = _idevice;
	(*idev)->AddRef();
}

const DocAccessData* Device::GetAccessData() const
{
	return _parent == 0 ? &_accessdata : this->GetRootDevice()->GetAccessData();
}

void Device::EnumerateDevices(IProcessDevice* iproc, void* param, int procid) const
{
	iproc->ProcessDevice(this, param, procid);
	
	for(DeviceIterator di = _devices.begin(); di != _devices.end(); ++di)
		(*di)->EnumerateDevices(iproc, param, procid);
}

void Device::GenerateFriendlyName()
{
	BSTR btmp = 0;

	if(_idevice->get_ModelName(&btmp) != S_OK)
	{
		SysFreeString(btmp);

		if(_idevice->get_FriendlyName(&btmp) != S_OK)
		{
			SysFreeString(btmp);

			btmp = SysAllocString(_udn.c_str());
		}
	}

	_name.assign(btmp);
	_name.append(L" (").append(_udn).append(L")");

	SysFreeString(btmp);
}

bool Device::SetUDN()
{
	bool result = false;
	BSTR btmp = 0;
	_idevice->get_UniqueDeviceName(&btmp);
	if(btmp != 0)
	{
		result = !_udn.assign(btmp).empty();
		SysFreeString(btmp);
	}
	return result;
}

wstring Device::GetDevDocAccessURL() const
{
	return _accessdata._url;
}

wstring Device::GetDeviceDocument() const
{
	return _accessdata._doc;
}

wstring Device::GetDocURL() const
{
	wstring result;
	HRESULT hr = S_OK;
	IUPnPDeviceDocumentAccess* idoc = 0;
	BSTR btmp = 0;

	hr = _idevice->QueryInterface(IID_IUPnPDeviceDocumentAccess, (void**)&idoc);

	if(hr == S_OK)
	{
		idoc->GetDocumentURL(&btmp);
		
		if(btmp != 0)
		{
			result.assign(btmp);

			SysFreeString(btmp);
		}
		
		idoc->Release();
	}

	return result;
}

bool Device::EnumDev()
{
	bool result = false;

	// enumerate and add services
	if(!EnumSrv())
		return result;

	IUPnPDevices* children = 0;
	VARIANT_BOOL bcheck = 0;

	// check if device has children
	if(_idevice->get_HasChildren(&bcheck) == S_OK)
	{
		if(!bcheck)
		{
			result = true; // no children, return without error
		}
		else
		{
			if(_idevice->get_Children(&children) == S_OK)
			{
				// device has children and collection's pointer is valid
				HRESULT hr = S_OK;
				IUnknown* ienum = 0;
				long devexpected = 0, devcount = 0;

				if(children->get_Count(&devexpected) == S_OK)
				{
					// enumerate and add devices
					if(children->get__NewEnum(&ienum) == S_OK)
					{
						IEnumUnknown* icol = 0;
						hr = ienum->QueryInterface(IID_IEnumUnknown, (void**)&icol);

						if(hr == S_OK)
						{
							IUnknown* iitem = 0;
							IUPnPDevice* ichild = 0;

							icol->Reset();
							while(icol->Next(1, &iitem, 0) == S_OK)
							{
								hr = iitem->QueryInterface(IID_IUPnPDevice, (void**)&ichild);

								if(hr == S_OK)
								{
									// create device object and add one to collection
									AddDevice(ichild);

									++devcount;

									ichild->Release();
									ichild = 0;
								}

								iitem->Release();
								iitem = 0;
							}

							icol->Release();
						}

						ienum->Release();
					}

					result = (devexpected == devcount);
				}

				children->Release();
			}
		}
	}

	return result;
}

bool Device::EnumSrv()
{
	bool result = false;

	HRESULT hr = S_OK;
	IUnknown* ienum = 0;
	IUPnPServices* isrvs = 0;
	long srvcount = 0, srvexpected = 0;


	if(_idevice->get_Services(&isrvs) == S_OK)
	{
		if(isrvs->get_Count(&srvexpected) == S_OK)
		{
			if(isrvs->get__NewEnum(&ienum) == S_OK)
			{
				IEnumUnknown* icol = 0;
				hr = ienum->QueryInterface(IID_IEnumUnknown, (void**)&icol);

				if(hr == S_OK)
				{
					IUnknown* iitem = 0;
					IUPnPService* isrv = 0;

					icol->Reset();
					while(icol->Next(1, &iitem, 0) == S_OK)
					{
						hr = iitem->QueryInterface(IID_IUPnPService, (void**)&isrv);

						if(hr == S_OK)
						{
							// add service to device object
							AddService(isrv);

							++srvcount;

							isrv->Release();
							isrv = 0;
						}

						iitem->Release();
						iitem = 0;
					}

					icol->Release();
				}

				ienum->Release();
			}

			result = (srvexpected == srvcount);
		}

		isrvs->Release();
	}

	return result;
}

bool Device::SetType()
{
	bool result = false;
	BSTR btmp = 0;

	_idevice->get_Type(&btmp);
	if(btmp != 0)
	{
		result = ! _type.assign(btmp).empty();
		SysFreeString(btmp);
	}

	return result;
}

bool Device::GetDeviceInfo(InfoData& data) const
{
	BSTR btmp = 0;
	int inscount = 0;

	data.insert(InfoDataItem(L"Friendly name", _name));
	++inscount;
	data.insert(InfoDataItem(L"Type", _type));
	++inscount;
	data.insert(InfoDataItem(L"UDN", _udn));
	++inscount;
	data.insert(InfoDataItem(L"Document URL", _accessdata._url));
	++inscount;

	_idevice->get_Description(&btmp);
	if(btmp != 0)
	{
		data.insert(InfoDataItem(L"Description", btmp));
		SysFreeString(btmp);
		btmp = 0;
		++inscount;
	}

	_idevice->get_ModelName(&btmp);
	if(btmp != 0)
	{
		data.insert(InfoDataItem(L"Model", btmp));
		SysFreeString(btmp);
		btmp = 0;
		++inscount;
	}

	_idevice->get_ManufacturerName(&btmp);
	if(btmp != 0)
	{
		data.insert(InfoDataItem(L"Manufacturer", btmp));
		SysFreeString(btmp);
		btmp = 0;
		++inscount;
	}

	_idevice->get_ManufacturerURL(&btmp);
	if(btmp != 0)
	{
		data.insert(InfoDataItem(L"Manufacturer URL", btmp));
		SysFreeString(btmp);
		btmp = 0;
		++inscount;
	}

	_idevice->get_ModelURL(&btmp);
	if(btmp != 0)
	{
		data.insert(InfoDataItem(L"Model URL", btmp));
		SysFreeString(btmp);
		btmp = 0;
		++inscount;
	}

	_idevice->get_ModelNumber(&btmp);
	if(btmp != 0)
	{
		data.insert(InfoDataItem(L"Model number", btmp));
		SysFreeString(btmp);
		btmp = 0;
		++inscount;
	}

	_idevice->get_SerialNumber(&btmp);
	if(btmp != 0)
	{
		data.insert(InfoDataItem(L"Serial number", btmp));
		SysFreeString(btmp);
		btmp = 0;
		++inscount;
	}

	_idevice->get_UPC(&btmp);
	if(btmp != 0)
	{
		data.insert(InfoDataItem(L"UPC", btmp));
		SysFreeString(btmp);
		btmp = 0;
		++inscount;
	}

	_idevice->get_PresentationURL(&btmp);
	if(btmp != 0)
	{
		data.insert(InfoDataItem(L"Presentation URL", btmp));
		SysFreeString(btmp);
		btmp = 0;
		++inscount;
	}

	return inscount == data.size();
}

wstring Device::GetDeviceIconURL(int iwidth, int iheight, int idepth) const
{
	wstring iconurl;

	if(!IsRoot())
		return iconurl;
	
	if(iwidth == 0 && iheight == 0)
	{
		// try get url for standard parameters trough interface
		// this method returns full uri

		BSTR btmp = 0;
		BSTR encformat = SysAllocString(L"image/png");

		if(encformat != 0)
		{
			_idevice->IconURL(encformat, 32, 32, 16, &btmp);

			SysFreeString(encformat);

			if(btmp != 0)
			{
				iconurl.assign(btmp);

				SysFreeString(btmp);
			}
		}
	}
	else
	{
		// try get icon as specified
		// url in scpd is relative path, so complete url with base url
		
		if(!_icons.empty())
		{
			for(list<IconParam>::const_iterator ipi = _icons.begin(); ipi != _icons.end(); ++ipi)
			{
				if((*ipi)._width == iwidth && (*ipi)._height == iheight)
				{
					if(idepth == 0 || idepth == (*ipi)._depth)
					{
						// complete url
						wstring missingslash;
						if(*(--_accessdata._urlbase.end()) != '/' && *((*ipi)._url.begin()) != '/')
							missingslash = L"/";
						iconurl.append(_accessdata._urlbase).append(missingslash).append((*ipi)._url);
						break;
					}
				}
			}
		}
	}

	return iconurl;
}



// *********************************************
// FindManager class
// *********************************************


const wchar_t* FindManager::device_type[] = 
{
	L"upnp:rootdevice",
	L"urn:schemas-upnp-org:device:Basic:1.0",
	L"urn:schemas-upnp-org:device:DigitalSecurityCamera:1",
	L"urn:schemas-upnp-org:device:HVAC_System:1",
	L"urn:schemas-upnp-org:device:InternetGatewayDevice:1",
	L"urn:schemas-upnp-org:device:BinaryLight:1",
	L"urn:schemas-upnp-org:device:DimmableLight:1",
	L"urn:schemas-upnp-org:device:WLANAccessPointDevice:1",
	L"urn:schemas-upnp-org:device:Printer:1",
	L"urn:schemas-upnp-org:device:RemoteUIClientDevice:1",
	L"urn:schemas-upnp-org:device:RemoteUIServerDevice:1",
	L"urn:schemas-upnp-org:device:Scanner:1",
	L"urn:schemas-upnp-org:device:MediaServer:2",
	L"urn:schemas-upnp-org:device:MediaRenderer:2",
	L"urn:schemas-upnp-org:device:MediaServer:1",
	L"urn:schemas-upnp-org:device:MediaRenderer:1"
};


FindManager::FindManager()
: _findercallback(0)
, _ifinder(0)
, _finderhandle(0)
, _findermanagerclient(0)
, _findercallbackclient(0)
, _srveventclient(0)
, _externalcollection(false)
{
	// Instantiate the device finder object
	HRESULT hr = CoCreateInstance(CLSID_UPnPDeviceFinder, 0, CLSCTX_SERVER, IID_IUPnPDeviceFinder, (void**)&_ifinder);
	if(hr != S_OK)
		throw invalid_argument("instantiating of finder object failed");

	if (::InitializeCriticalSectionAndSpinCount(&_cs, 4000) == FALSE)
		throw std::exception("initialize critical section failed");
}

FindManager::~FindManager()
{
	Stop();
	_finderhandle = 0;
	_ifinder->Release();
	ReleaseCallback();

	// delete device objects
	RemoveAllDevices();

	::DeleteCriticalSection(&_cs);
}

wstring FindManager::GetRootDeviceType()
{
	return device_type[0];
}

int FindManager::TypesCount()
{
	return sizeof(device_type) / sizeof(const wchar_t*);
}

// internal collection
bool FindManager::Init(IFinderManagerClient* client, const wstring& devicetype/* = L"upnp:rootdevice"*/)
{
	if(client == 0 || devicetype.empty())
		return false;

	::EnterCriticalSection(&_cs);

	_findermanagerclient = client;
	_findercallbackclient = 0;
	_externalcollection = false;

	bool result = false;
	try
	{
		result = Init(devicetype);
	}
	catch(std::exception)
	{
	}

	::LeaveCriticalSection(&_cs);
	
	return result;
}

// external collection
bool FindManager::Init(IFinderCallbackClient* client, const wstring& devicetype/* = L"upnp:rootdevice"*/)
{
	if(client == 0 || devicetype.empty())
		return false;

	::EnterCriticalSection(&_cs);

	_findermanagerclient = 0;
	_findercallbackclient = client;
	_externalcollection = true;

	bool result = false;
	try
	{
		result = Init(devicetype);
	}
	catch(std::exception)
	{
	}

	::LeaveCriticalSection(&_cs);
	
	return result;
}

bool FindManager::Init(const wstring& devicetype)
{
	bool result = false;

	// release resources
	Stop();
	_finderhandle = 0;
	ReleaseCallback();

	RemoveAllDevices();

	BSTR devtype = SysAllocString(devicetype.c_str());

	if(devtype != 0)
	{
		// create callback
		// callback object is deleting in FindManager::ReleaseCallback() in DevFinderCallback::Release()
		_findercallback = new DevFinderCallback(_externalcollection ? _findercallbackclient : this);
		_findercallback->AddRef();

		result = _ifinder->CreateAsyncFind(devtype, 0, _findercallback, &_finderhandle) == S_OK;
	}

	SysFreeString(devtype);

	return result;
}

void FindManager::ReleaseCallback()
{
	if(_findercallback != 0)
	{
		_findercallback->Release(); // release created in CreateCallback
		_findercallback = 0;
	}
}

bool FindManager::Start()
{
	bool result = false;
	HRESULT hr;

	if(_finderhandle != 0)
	{
		hr = _ifinder->StartAsyncFind(_finderhandle);
		if(hr == S_OK)
		{
			result = true;

			// notify manager's client
			if(!_externalcollection)
				_findermanagerclient->OnStartFindDevice(_finderhandle);
		}
	}

	return result;
}

bool FindManager::Stop()
{
	bool result = false;
	HRESULT hr;

	if(_finderhandle != 0)
	{
		hr = _ifinder->CancelAsyncFind(_finderhandle);
		if(hr == S_OK) 
		{
			result = true;

			// notify client
			if(!_externalcollection)
				_findermanagerclient->OnStopFindDevice(_finderhandle, true);
		}
	}

	return result;
}

const DeviceArray* FindManager::GetCollection() const
{
	if(_externalcollection)
		throw invalid_argument("collection is managed externally");
	else
		return &_devs;
}

DeviceArrayIterator FindManager::GetCollectionBegin() const
{
	if(_externalcollection)
		throw invalid_argument("collection is managed externally");
	else
		return _devs.begin();
}

DeviceArrayIterator FindManager::GetCollectionEnd() const
{
	if(_externalcollection)
		throw invalid_argument("collection is managed externally");
	else
		return _devs.end();
}

const Device* FindManager::GetDevice(unsigned int index) const
{
	if(_externalcollection)
		throw invalid_argument("collection is managed externally");

	if(index < 0 || index >= _devs.size())
		throw invalid_argument("index out of range");

	return _devs[index];
}

const Device* FindManager::operator[] (unsigned int index) const
{
	return GetDevice(index);
}

int FindManager::GetCollectionCount() const
{
	if(_externalcollection)
		return 0;
	else
		return _devs.size();
}

bool FindManager::IsCollectionEmpty() const
{
	return _devs.empty();
}

long FindManager::GetFindId()
{
	return _finderhandle;
}

void FindManager::SetServiceEventClientPtr(IServiceCallbackClient* client)
{
	_srveventclient = client;
}

void FindManager::Lock()
{
	::EnterCriticalSection(&_cs);
}

void FindManager::UnLock()
{
	::LeaveCriticalSection(&_cs);
}

void FindManager::ProcessDevice(const Device* dev, void* param, int procid)
{
	ServiceIterator si = dev->GetServiceListBegin();
	int srvcount = dev->GetServiceListCount();

	for(int i = 0; i < srvcount; ++i, ++si)
		(*si)->SetCallbackClient(_srveventclient);
}

void FindManager::DeviceAdded(long findid, IUPnPDevice* idev)
{
	if(findid != _finderhandle)
		return;

	// create new root Device object and build its structure
	Device* dev = new Device(idev);

	// add callback for services events
	if(_srveventclient != 0)
		dev->EnumerateDevices(this, 0, 0);

	// add Device root object to collection
	_devs.push_back(dev);

	// notify client
	if(!_externalcollection && _findermanagerclient != 0)
		_findermanagerclient->OnAddDevice(findid, dev, _devs.size() - 1);
}

void FindManager::DeviceRemoved(long findid, const wstring& devname)
{
	if(findid != _finderhandle)
		return;

	int i = 0;
	for(vector<Device*>::iterator di = _devs.begin(); di < _devs.end(); ++di, ++i)
	{
		if(devname == (*di)->GetUDN())
		{
			wstring friendlyname = (*di)->GetFriendlyName();

			// remove passed device from devices collection
			delete *di;
			*di = 0;
			_devs.erase(di);

			// notify client
			if(!_externalcollection && _findermanagerclient != 0)
				_findermanagerclient->OnRemoveDevice(findid, devname, friendlyname, i);

			break;
		}
	}
}

void FindManager::SearchComplete(long findid)
{
	if(findid != _finderhandle)
		return;

	// notify client
	if(!_externalcollection && _findermanagerclient != 0)
		_findermanagerclient->OnStopFindDevice(findid, false);
}

void FindManager::RemoveAllDevices()
{
	if(!_devs.empty())
	{
		for(vector<Device*>::iterator di = _devs.begin(); di != _devs.end(); ++di)
		{
			delete *di;
			*di = 0;
		}

		_devs.clear();
	}
}



// *********************************************
// UPnP helper functions
// *********************************************


wstring UPnPCpLib::GetErrorMessage(HRESULT hr)
{
	wstring errout(L"Unknown error");

	// standard device error code
	switch(hr)
	{
	case UPNP_E_ROOT_ELEMENT_EXPECTED:
		errout = L"Root Element Expected";
		break;
	case UPNP_E_DEVICE_ELEMENT_EXPECTED:
		errout = L"Device Element Expected";
		break;
	case UPNP_E_SERVICE_ELEMENT_EXPECTED:
		errout = L"Service Element Expected";
		break;
	case UPNP_E_SERVICE_NODE_INCOMPLETE:
		errout = L"Service Node Incomplete";
		break;
	case UPNP_E_DEVICE_NODE_INCOMPLETE:
		errout = L"Device Node Incomplete";
		break;
	case UPNP_E_ICON_ELEMENT_EXPECTED:
		errout = L"Icon Element Expected";
		break;
	case UPNP_E_ICON_NODE_INCOMPLETE:
		errout = L"Icon Node Incomplete";
		break;
	case UPNP_E_INVALID_ACTION:
		errout = L"Invalid Action";
		break;
	case UPNP_E_INVALID_ARGUMENTS:
		errout = L"Invalid Arguments";
		break;
	case UPNP_E_OUT_OF_SYNC:
		errout = L"Out of Sync";
		break;
	case UPNP_E_ACTION_REQUEST_FAILED:
		errout = L"Action Request Failed";
		break;
	case UPNP_E_TRANSPORT_ERROR:
		errout = L"Transport Error";
		break;
	case UPNP_E_VARIABLE_VALUE_UNKNOWN:
		errout = L"Variable Value Unknown";
		break;
	case UPNP_E_INVALID_VARIABLE:
		errout = L"Invalid Variable";
		break;
	case UPNP_E_DEVICE_ERROR:
		errout = L"Device Error";
		break;
	case UPNP_E_PROTOCOL_ERROR:
		errout = L"Protocol Error";
		break;
	case UPNP_E_ERROR_PROCESSING_RESPONSE:
		errout = L"Error Processing Response";
		break;
	case UPNP_E_DEVICE_TIMEOUT:
		errout = L"Device Timeout";
		break;
	case UPNP_E_INVALID_DOCUMENT:
		errout = L"Invalid Document";
		break;
	case UPNP_E_EVENT_SUBSCRIPTION_FAILED:
		errout = L"Event Subscription Failed";
		break;
	default:
		// non-standard device error code
		if((hr >= UPNP_E_ACTION_SPECIFIC_BASE) && (hr <= UPNP_E_ACTION_SPECIFIC_MAX))
			errout = L"Action Specific Error";
		break;
	}

	return errout;
}

VARTYPE UPnPCpLib::GetVariantType(const wstring& vtype)
{
	VARTYPE vt = VT_BSTR;
	wstring _vtype(vtype);

	transform(_vtype.begin(), _vtype.end(), _vtype.begin(), tolower);

	if((_vtype.compare(L"string") == 0) || (_vtype.compare(L"uri") == 0) || (_vtype.compare(L"uuid") == 0))
		vt = VT_BSTR;
	else if(_vtype.compare(L"boolean") == 0)
		vt = VT_BOOL;
	else if(_vtype.compare(L"ui1") == 0)
		vt = VT_UI1;
	else if((_vtype.compare(L"ui2") == 0) || (_vtype.compare(L"char") == 0))
		vt = VT_UI2;
	else if(_vtype.compare(L"ui4") == 0)
		vt = VT_UI4;
	else if(_vtype.compare(L"i1") == 0)
		vt = VT_I1;
	else if(_vtype.compare(L"i2") == 0)
		vt = VT_I2;
	else if(_vtype.compare(L"i4") == 0)
		vt = VT_I4;
	else if(_vtype.compare(L"int") == 0)
		vt = VT_INT;
	else if((_vtype.compare(L"r4") == 0) || (_vtype.compare(L"float") == 0))
		vt = VT_R4;
	else if((_vtype.compare(L"r8") == 0) || (_vtype.compare(L"number") == 0))
		vt = VT_R8;
	else if((_vtype.compare(L"date") == 0) || (_vtype.compare(L"dateTime") == 0) || (_vtype.compare(L"dateTime.tz") == 0) || (_vtype.compare(L"time") == 0) || (_vtype.compare(L"time.tz") == 0))
		vt = VT_DATE;
	else if((_vtype.compare(L"bin.base64") == 0) || (_vtype.compare(L"bin.hex") == 0))
		vt = VT_ARRAY | VT_UI1;
	else if(_vtype.compare(L"fixed.14.4") == 0)
		vt = VT_CY;

	return vt;
}

wstring UPnPCpLib::GetTypeDescr(VARTYPE vtype)
{
	wstring result;

	switch(vtype)
	{
	case VT_BSTR:
		result = L"string";
		break;
	case VT_BOOL:
		result = L"boolean";
		break;
	case VT_UI1:
		result = L"ui1";
		break;
	case VT_UI2:
		result = L"ui2";
		break;
	case VT_UI4:
		result = L"ui4";
		break;
	case VT_I1:
		result = L"i1";
		break;
	case VT_I2:
		result = L"i2";
		break;
	case VT_I4:
		result = L"i4";
		break;
	case VT_R4:
		result = L"r4";
		break;
	case VT_R8:
		result = L"r8";
		break;
	case VT_INT:
		result = L"int";
		break;
	case VT_DATE:
		result = L"date";
		break;
	case VT_ARRAY | VT_UI1:
		result = L"bin.base64";
		break;
	case VT_CY:
		result = L"fixed.14.4";
		break;
	}

	return result;
}

bool UPnPCpLib::IsICSConnEnabled()
{
	HRESULT hr = S_OK;

	hr = CoInitializeSecurity(0, -1, 0, 0, RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_NONE, 0);
	if(hr != S_OK && hr != RPC_E_TOO_LATE)
		return false;

	long pubconns = 0;
	long prvconns = 0;
	bool pubactive = false;
	bool prvactive = false;

	INetSharingManager* csmgr = 0;
	hr = CoCreateInstance(CLSID_NetSharingManager, 0, CLSCTX_INPROC_SERVER, IID_INetSharingManager, (LPVOID*)&csmgr);
	if(hr == S_OK)
	{
		VARIANT_BOOL isinstalled = 0; // false
		csmgr->get_SharingInstalled(&isinstalled);
		if(isinstalled)
		{
			ULONG lfetched = 0;
			IUnknown* ienum = 0;
			VARIANT vconn;
			INetConnection* iconn = 0;

			INetSharingPublicConnectionCollection* ipubcol = 0;
			hr = csmgr->get_EnumPublicConnections(ICSSC_DEFAULT, &ipubcol);
			if(hr == S_OK)
			{
				if(ipubcol->get_Count(&pubconns) == S_OK && pubconns > 0)
				{
					if(ipubcol->get__NewEnum(&ienum) == S_OK)
					{
						IEnumNetSharingPublicConnection* ipubs = 0;
						hr = ienum->QueryInterface(IID_IEnumNetSharingPublicConnection, (void**)&ipubs);
						if(hr == S_OK)
						{
							VariantInit(&vconn);
							ipubs->Reset();
							while(ipubs->Next(1, &vconn, &lfetched) == S_OK)
							{
								pubactive = true;
								/*iconn = 0;
								iconn = (INetConnection*)vconn.punkVal;*/
								VariantClear(&vconn);
							}
							
							ipubs->Release();
						}

						ienum->Release();
						ienum = 0;
					}
				}
				
				ipubcol->Release();
			}

			INetSharingPrivateConnectionCollection* iprvcol = 0;
			hr = csmgr->get_EnumPrivateConnections(ICSSC_DEFAULT, &iprvcol);
			if(hr == S_OK)
			{
				if(iprvcol->get_Count(&prvconns) == S_OK && prvconns > 0)
				{
					if(iprvcol->get__NewEnum(&ienum) == S_OK)
					{
						IEnumNetSharingPrivateConnection* iprvs = 0;
						hr = ienum->QueryInterface(IID_IEnumNetSharingPrivateConnection, (void**)&iprvs);
						if(hr == S_OK)
						{
							VariantInit(&vconn);
							iprvs->Reset();
							while(iprvs->Next(1, &vconn, &lfetched) == S_OK)
							{
								prvactive = true;
								/*iconn = 0;
								iconn = (INetConnection*)vconn.punkVal;*/
								VariantClear(&vconn);
							}
							
							iprvs->Release();
						}
						
						ienum->Release();
						ienum = 0;
					}
				}
				
				iprvcol->Release();
			}
		}
		
		csmgr->Release();
	}

	return (pubconns && prvconns) && (pubactive && prvactive);
}

bool UPnPCpLib::CheckServiceState(const wstring& srvname, /*out*/srvstate& state)
{
	bool result = false;
	SC_HANDLE hscm = 0;
	hscm = OpenSCManagerW(0, 0, SC_MANAGER_CONNECT);
	if(hscm == 0)
		return result;

	SC_HANDLE hsrv = 0;
	hsrv = OpenServiceW(hscm, srvname.c_str(), SERVICE_ALL_ACCESS);
	if(hsrv != 0)
	{
		SERVICE_STATUS srvstatus;

		if(QueryServiceStatus(hsrv, &srvstatus))
		{
			result = true;

			switch(srvstatus.dwCurrentState)
			{
			case SERVICE_START_PENDING :
				state = ss_start_pending;
				break;
			case SERVICE_STOP_PENDING :
				state = ss_stop_pending;
				break;
			case SERVICE_PAUSE_PENDING :
				state = ss_pause_pending;
				break;
			case SERVICE_CONTINUE_PENDING :
				state = ss_continue_pending;
				break;
			case SERVICE_RUNNING :
				state = ss_running;
				break;
			case SERVICE_STOPPED :
				state = ss_stopped;
				break;
			case SERVICE_PAUSED :
				state = ss_paused;
				break;
			}
		}
		
		CloseServiceHandle(hsrv);
	}

	CloseServiceHandle(hscm);

	return result;
}

bool UPnPCpLib::ControlSSDPService(bool start)
{
	bool result = false;
	
	SC_HANDLE hscm = 0;
	hscm = OpenSCManagerW(0, 0, SC_MANAGER_CONNECT);
	if(hscm == 0)
		return result;

	SC_HANDLE hsrv = 0;
	hsrv = OpenServiceW(hscm, L"ssdpsrv", SERVICE_ALL_ACCESS);
	if(hsrv != 0)
	{
		SERVICE_STATUS srvstatus;

		if(QueryServiceStatus(hsrv, &srvstatus))
		{
			HANDLE hevent = CreateEventW(0, true, false, L"dummyevent");
			
			// if service_*_pending then wait for complete operation
			switch(srvstatus.dwCurrentState)
			{
			case SERVICE_START_PENDING :
			case SERVICE_STOP_PENDING :
				for(int i = 0; i < 10; ++i)
				{
					if(!QueryServiceStatus(hsrv, &srvstatus))
						break;
					if(srvstatus.dwCurrentState == SERVICE_RUNNING || srvstatus.dwCurrentState == SERVICE_STOPPED)
						break;
					WaitForSingleObject(hevent, 1000);
				}
				break;
			}

			// operation completed, so change current state
			if(QueryServiceStatus(hsrv, &srvstatus))
			{
				switch(srvstatus.dwCurrentState)
				{
				case SERVICE_RUNNING : // stop it
					if(!start && ControlService(hsrv, SERVICE_CONTROL_STOP, &srvstatus))
						result = true;
					break;
				case SERVICE_STOPPED : // start it
					if(start && StartServiceW(hsrv, 0, 0))
						result = true;
					break;
				}

				// wait for complete operation
				if(result)
				{
					for(int i = 0; i < 10; ++i)
					{
						if(!QueryServiceStatus(hsrv, &srvstatus))
							break;
						if(srvstatus.dwCurrentState == SERVICE_RUNNING || srvstatus.dwCurrentState == SERVICE_STOPPED)
						{
							// notify scm
							//ControlService(hsrv, SERVICE_CONTROL_INTERROGATE, &srvstatus);
							break;
						}
						WaitForSingleObject(hevent, 1000);
					}
				}
			}
		}
		
		CloseServiceHandle(hsrv);
	}

	CloseServiceHandle(hscm);
	
	return result;
}

int UPnPCpLib::CheckFirewallPortState(long number, transport_protocol protocol)
{
	int result = 0; // error

	INetFwMgr*		imgr = 0;
	INetFwPolicy*	ipol = 0;
	INetFwProfile*	iprof = 0;

	HRESULT hr = S_OK;
	VARIANT_BOOL portenabled = 0; // false

	hr = CoCreateInstance(__uuidof(NetFwMgr), 0, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&imgr);
	if(hr != S_OK)
		return 0;

	hr = S_FALSE;

	if(imgr->get_LocalPolicy(&ipol) == S_OK)
	{
		if(ipol->get_CurrentProfile(&iprof) == S_OK)
		{
			INetFwOpenPorts* iports = 0;
			if(iprof->get_GloballyOpenPorts(&iports) == S_OK)
			{
				NET_FW_IP_PROTOCOL proto;
				switch(protocol)
				{
				case tcp_protocol:
					proto = NET_FW_IP_PROTOCOL_TCP;
					break;
				case udp_protocol:
					proto = NET_FW_IP_PROTOCOL_UDP;
					break;
				default:
					proto = NET_FW_IP_PROTOCOL_TCP;
				}

				INetFwOpenPort* iport = 0;
				
				hr = iports->Item(number, proto, &iport);
				if(hr == S_OK)
				{
					hr = iport->get_Enabled(&portenabled);
					iport->Release();
				}
				
				iports->Release();
			}
			
			iprof->Release();
		}
		
		ipol->Release();
	}
	
	imgr->Release();

	if(hr == S_OK)
		result = portenabled ? 1 : -1;

	return result;
}

bool UPnPCpLib::ControlUPnPPorts(bool open)
{
	INetFwMgr*		imgr = 0;
	INetFwPolicy*	ipol = 0;
	INetFwProfile*	iprof = 0;

	HRESULT hr = S_OK;
	bool port2869 = false;
	bool port1900 = false;

	hr = CoCreateInstance(__uuidof(NetFwMgr), 0, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&imgr);
	if(hr != S_OK)
		return false;

	if(imgr->get_LocalPolicy(&ipol) == S_OK)
	{
		if(ipol->get_CurrentProfile(&iprof) == S_OK)
		{
			INetFwOpenPorts* iports = 0;
			if(iprof->get_GloballyOpenPorts(&iports) == S_OK)
			{
				INetFwOpenPort* iport = 0;
				VARIANT_BOOL portenabled = open ? -1 : 0;
				BSTR btmp = 0;
				
				hr = iports->Item(2869L, NET_FW_IP_PROTOCOL_TCP, &iport);
				if(hr != S_OK)
				{
					hr = CoCreateInstance(__uuidof(NetFwOpenPort), 0, CLSCTX_INPROC_SERVER, __uuidof(INetFwOpenPort), (void**)&iport);
					if(hr == S_OK)
					{
						hr = S_FALSE;
						btmp = SysAllocString(L"UPnP TCP 2869");
						if(btmp != 0)
						{
							if(iport->put_Name(btmp) == S_OK)
								if(iport->put_Port(2869L) == S_OK)
									if(iport->put_Protocol(NET_FW_IP_PROTOCOL_TCP) == S_OK)
										if(iport->put_Scope(NET_FW_SCOPE_LOCAL_SUBNET) == S_OK)
											hr = iports->Add(iport);

							SysFreeString(btmp);
							btmp = 0;
						}
					}
				}
				if(hr == S_OK && iport->put_Enabled(portenabled) == S_OK)
					port2869 = true;
				
				if(iport)
					iport->Release();

				hr = iports->Item(1900L, NET_FW_IP_PROTOCOL_UDP, &iport);
				if(hr != S_OK)
				{
					hr = CoCreateInstance(__uuidof(NetFwOpenPort), 0, CLSCTX_INPROC_SERVER, __uuidof(INetFwOpenPort), (void**)&iport);
					if(hr == S_OK)
					{
						hr = S_FALSE;
						btmp = SysAllocString(L"UPnP UDP 1900");
						if(btmp != 0)
						{
							if(iport->put_Name(btmp) == S_OK)
								if(iport->put_Port(1900L) == S_OK)
									if(iport->put_Protocol(NET_FW_IP_PROTOCOL_UDP) == S_OK)
										if(iport->put_Scope(NET_FW_SCOPE_LOCAL_SUBNET) == S_OK)
											hr = iports->Add(iport);

							SysFreeString(btmp);
						}
					}
				}
				if(hr == S_OK && iport->put_Enabled(portenabled) == S_OK)
					port1900 = true;

				if(iport)
					iport->Release();
				
				iports->Release();
			}
			
			iprof->Release();
		}
		
		ipol->Release();
	}
	
	imgr->Release();

	return port2869 && port1900;
}

