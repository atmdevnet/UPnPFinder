/*****************************************************/
/*  UPnPCPLib library                                */
/*  UPnP Control Point API client                    */
/*  STL version                                      */
/*                                                   */
/*  Attention:                                       */
/*  specify MARKUP_STL preprocessor definition       */
/*  in project properties                            */
/*****************************************************/

#ifndef __UPnPCPLib_h__
#define __UPnPCPLib_h__

#include <string>
#include <list>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <map>

using std::string;
using std::wstring;
using std::list;
using std::vector;
using std::map;
using std::pair;
using std::invalid_argument;
using std::find;
using std::transform;

// conversion from size_t to int in VS 2003
#pragma warning(disable : 4267)

// Callback classes receiving events from UPnP framework
// (SrvEventCallback and DevFinderCallback) can use
// multi or single -threading concurrency model thus
// thus you can initialize COM library with CoInitializeEx
// specifying concurrency model flag as 
// COINIT_MULTITHREADED or COINIT_APARTMENTTHREADED respectively.                
// To change threading model to single threaded 
// you should initialize COM specifying         
// COINIT_APARTMENTTHREADED flag and comment    
// following line.                              
#define UCPL_MULTITHREADED


// for CoInitializeSecurity and InitializeCriticalSectionAndSpinCount
// if _WIN32_WINNT is defined make sure that its value is equal or greater than 0x0403
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0403
#endif


// Winsock2 API
#include <winsock2.h>
#pragma comment(lib, "ws2_32")

// UPnP API
#include <upnp.h>

// TCHAR API
#include <tchar.h>

// CMarkup
// in non-MFC build CMarkup requires preprocessor definition - MARKUP_STL
#include "markup.h"

// ICS API
#include <netcon.h>

// System services API
#include <winsvc.h>
#pragma comment(lib, "advapi32")

// Windows Firewall API
#include <Netfw.h>

// _beginthread
#include <process.h>

// OLE automation functions
#pragma comment(lib, "oleaut32")
// CoInitializeEx and CoInitializeSecurity
#pragma comment(lib, "ole32")


namespace UPnPCpLib
{


// for arguments of CMarkup functions in Unicode and Multibyte builds
#if defined(UNICODE) || defined(_UNICODE)
	typedef wstring mstring;
#else
	typedef string mstring;
#endif

	
// forward declarations
class Action;
class Service;
class Device;
class FindManager;


// for exchange and presentation objects data
typedef map<wstring, wstring> InfoData;
typedef pair<wstring, wstring> InfoDataItem;
typedef list<InfoData> InfoDataList;

// iterator for InfoData
typedef InfoData::const_iterator InfoIterator;

// state variable collection
typedef map<wstring, bool> VarData;
typedef pair<wstring, bool> VarDataItem;

// iterator for InfoData
typedef VarData::const_iterator VarIterator;

// types dedicated to passing arguments of actions
typedef vector<InfoDataItem> ArgsArray;
typedef ArgsArray::const_iterator ArgsIterator;

// simple list of strings
typedef list<wstring> StrList;
typedef StrList::const_iterator StrIterator;


// types for icons collection
struct IconParam
{
	IconParam();
	IconParam(const wstring& vurl, const wstring& vmime, int vwidth, int vheight, int vdepth);
	IconParam(const IconParam& srcobj);

	IconParam& operator= (const IconParam& srcobj);

	wstring	_url;
	wstring	_mime;
	int		_width;
	int		_height;
	int		_depth;
};

typedef list<IconParam> IconList;
typedef IconList::const_iterator IconIterator;


// simply list of actions used by Service class
typedef list<Action> ActionList;

// iterator for lists of strings
typedef ActionList::const_iterator ActionIterator;

// service objects list used by Device class
typedef list<Service*> ServiceList;
// device objects list used by Device class
typedef list<Device*> DeviceList;

// iterators for above lists
typedef ServiceList::const_iterator ServiceIterator;
typedef DeviceList::const_iterator DeviceIterator;

// Device's collection
typedef vector<Device*> DeviceArray;
// iterator for Device's collection
typedef DeviceArray::const_iterator DeviceArrayIterator;


// state of system services
enum srvstate
{
	ss_start_pending,		// Start pending
	ss_stop_pending,		// Stop pending
	ss_pause_pending,		// Pause pending
	ss_continue_pending,	// Continue pending
	ss_running,				// Running
	ss_stopped,				// Stopped
	ss_paused				// Paused
};


// protocols identifiers
enum transport_protocol
{
	any_protocol,
	tcp_protocol,
	udp_protocol
};


// helper types for invoking handlers of upnp events
struct EventData;

struct IEventInvoke
{
	virtual void InvokeClientEvent(const EventData* data) = 0;
};

struct EventData
{
	enum FunctionSwitch
	{
		DeviceAdded,
		DeviceRemoved,
		SearchComplete,
		StateVariableChanged,
		ServiceInstanceDied
	};

	EventData(IEventInvoke* client, FunctionSwitch fswitch, long fid, IUPnPDevice* idev);
	EventData(IEventInvoke* client, FunctionSwitch fswitch, long fid, const wstring& devudn);
	EventData(IEventInvoke* client, FunctionSwitch fswitch, long fid);
	EventData(IEventInvoke* client, FunctionSwitch fswitch, const Service* srv, const wstring& varname, const wstring& varvalue);
	EventData(IEventInvoke* client, FunctionSwitch fswitch, const Service* srv);

	IEventInvoke*	_client;
	FunctionSwitch	_switch;
	long			_findid;
	IUPnPDevice*	_idev;
	wstring			_devudn;
	const Service*	_srv;
	wstring			_varname;
	wstring			_varvalue;
};


// ============== DocAccessData struct ============== //


// data for manipulating description documents
struct DocAccessData
{
	DocAccessData();
	
	// ctor for device objects, calls SetData
	// to collect data for service use member functions
	DocAccessData(const wstring& url);

	// collects DocAccessData members for device object
	// on input must pass valid descr doc url of device
	bool SetData(const wstring& url);

	// retrieves inet address & path from access url address string
	// _url must be set
	bool SetAddress();

	// tries retrieve base url from access url
	// _url must be set
	bool SetBaseURL();

	// retrieves tag value from document at root level
	// _doc must be set
	bool GetXmlDataRoot(const wstring& tag, /*out*/wstring& value) const;

	// retrieves url of service's descr doc from document
	// _doc must be set
	bool GetXmlDataScpdUrl(const wstring& srvid, /*out*/wstring& scpdurl) const;

	// retrieves state variables list from scpd document
	// _doc must be set
	bool GetXmlDataVariables(/*out*/InfoData& varsinfo) const;

	// retrieves actions list from scpd document
	// _doc must be set
	bool GetXmlDataActions(/*out*/StrList& actlist) const;

	// retrieves action's arguments from scpdurl's content
	// _doc must be set
	bool GetXmlDataActionArgs(const wstring& aname, /*out*/InfoDataList& inflist) const;

	// retrieves number of action's arguments from scpdurl's content
	// _doc must be set
	bool GetXmlDataActionArgsCount(const wstring& aname, /*out*/int& argcount) const;

	// retrieves info about state variable
	bool GetXmlDataActionVarInfo(const wstring& vname, /*in/out*/InfoData& infdata) const;

	// retrieves list of icons of device
	bool GetXmlDataIconsList(/*out*/IconList& icolist) const;

	// downloads requested resource from device to document
	// _addr and _path must be set
	// timeout for function select, default 100 ms
	bool LoadData(long mili_seconds = 100L);


	sockaddr_in	_addr;		// winsock host address
	wstring		_path;		// path to resource on host
	wstring		_url;		// description document uri
	wstring		_urlbase;	// common base part of uri
	wstring		_doc;		// content of description document
};


// ============== Action class ============== //


// class representing member action of UPnP service
class Action
{
	friend class Service;

public:
	// name of this action
	wstring GetName() const;
	
	const Service& GetParentService() const;

	// number of arguments (input & output)
	// returns -1 if error occured
	int GetArgsCount();
	// number of input arguments
	// returns -1 if error occured
	int GetInArgsCount();

	// retrieves this action info
	bool GetInfo(/*out*/InfoDataList& inflist) const;

	// sets array of input arguments types and values
	bool SetInArgs(const StrList& args);
	// sets value of argument at specified index in input arguments array
	bool SetInArgs(const wstring& arg, unsigned int index);
	// gets array of input arguments types and values
	bool GetInArgs(ArgsArray& args);
	// gets input argument at specified index in input arguments array
	bool GetInArgs(InfoDataItem& arg, unsigned int index);

	// invokes this action on parent UPnP service
	// argsout	- array of values of output arguments or error message.
	//			  on error argsout contains only error messages of InvokeAction
	// return:	-1 = error, >0 = number of output arguments + returned value (if any)
	//			-2 = argsout contains only returned value
	int Invoke(ArgsArray& argsout) const;

private:
	wstring			_name;				// action's name
	const Service*	_parent;			// this action's parent service
	int				_argcount;			// number of input arguments
	int				_inargcount;		// number of all arguments, input and output
	bool			_complete;			// true if number of arguments has been retrieved
	bool			_inargscomplete;	// true if number of input arguments has been retrieved
	ArgsArray		_in;				// array of input arguments (types and values)

	bool SetArgsCount();
	bool InitInArgsList();

	Action(const Service* srv, const wstring& name);
};


// ============== IUPnPService callback ============== //


// interface for communication with client related to services events
struct IServiceCallbackClient
{
	virtual void ServiceEventVariableChanged(const Service* srv, const wstring& varname, const wstring& varvalue) = 0;
	virtual void ServiceEventInstanceDied(const Service* srv) = 0;
};


// IUPnPServiceCallback implementation
class SrvEventCallback : public IUPnPServiceCallback, public IEventInvoke
{
	friend class Service;

private:
	long					_refcount;
	IServiceCallbackClient* _client;
	Service*				_host;

public:
	virtual ~SrvEventCallback() {}

	// IEventInvoke implementation
	virtual void InvokeClientEvent(const EventData* data);

	// IUnknown implementation
	virtual HRESULT _stdcall QueryInterface(const IID& riid, void** ppvObject);
	virtual unsigned long _stdcall AddRef();
	virtual unsigned long _stdcall Release();

	// IUPnPServiceCallback implementation
	virtual HRESULT _stdcall StateVariableChanged(IUPnPService* isrv, LPCWSTR varname, VARIANT varvalue);
	virtual HRESULT _stdcall ServiceInstanceDied(IUPnPService* isrv);

	void SetClientPtr(IServiceCallbackClient* client);

private:
	explicit SrvEventCallback(Service* host);

	SrvEventCallback(const SrvEventCallback& srcobj);
	SrvEventCallback& operator= (const SrvEventCallback& srcobj);
};


// ============== Service class ============== //


// class representing UPnP service
class Service
{
	friend class Device;

public:
	~Service();

	// access to list of actions
	// each service may have 0 or more actions
	int GetActionCount() const;
	ActionIterator GetCollectionBegin() const;
	ActionIterator GetCollectionEnd() const;
	const Action& GetAction(unsigned int index) const;
	const Action& GetAction(const wstring& aname) const;

	wstring GetServiceID() const;
	wstring GetServiceTypeID() const;

	// if returns zero then error occurred
	long GetLastTransportStatus() const;

	const Device& GetParentDevice() const;

	// with AddRef, don't forget to release interface when unused
	void GetInterface(IUPnPService** isrv) const;

	// adds callback for events and sets its client
	bool SetCallbackClient(IServiceCallbackClient* iclient);

	// returns descr document url of this service
	wstring GetScpdURL() const;
	// returns descr document content of this service
	wstring GetScpdContent() const;
	// returns structure contains data which helps manipulate descr documents
	const DocAccessData* GetAccessData() const;

	// retrieves info about this UPnP service
	bool GetServiceInfo(InfoData& data) const;

	// retrieves info about service's state variables
	// each service must have one or more state variables
	bool GetServiceVariables(VarData& data) const;

private:
	Service(IUPnPService* isrv, const Device& parentdev);

	// retrieves scpd info, url and document content
	bool SetAccessData();

	// reads actions names
	bool EnumActions();

	bool SetServiceID();

	wstring				_name;			// service Id
	wstring				_typeid;		// service type Id
	ActionList			_actions;		// list of UPnP service actions
	const Device&		_parent;		// parent device object
	IUPnPService*		_iservice;		// COM interface of UPnP service
	DocAccessData		_accessdata;	// uri and content of document describing UPnP service
	SrvEventCallback*	_isrvcback;		// IUPnPServiceCallback object

	Service(const Service& srcobj);
	Service& operator= (const Service& srcobj);
};


// ============== Device class ============== //


// interface used to process tree of device objects.
// implement this interface in class that will be receiving,
// in member function ProcessDevice, pointer to current device object
// from enumerated tree of devices.
struct IProcessDevice
{
	// dev		- Device object to process
	// param	- any type custom parameter
	// procid	- custom parameter to help control behavior of processing method
	// values of param & procid can be 0 if not used

	virtual void ProcessDevice(const Device* dev, void* param, int procid) = 0;
};


// class representing UPnP device
class Device
{
public:
	explicit Device(IUPnPDevice* idev, const Device* parentdev = 0);
	~Device();

	wstring GetUDN() const;
	wstring GetFriendlyName() const;
	wstring GetType() const;

	// access to list of services
	int GetServiceListCount() const;
	ServiceIterator GetServiceListBegin() const;
	ServiceIterator GetServiceListEnd() const;
	const Service* GetService(unsigned int index) const;

	// access to list of devices
	int GetDeviceListCount() const;
	DeviceIterator GetDeviceListBegin() const;
	DeviceIterator GetDeviceListEnd() const;
	const Device* GetDevice(unsigned int index) const;
	bool IsDeviceListEmpty() const;

	// access to list of icons
	int GetIconsListCount() const;
	IconIterator GetIconsListBegin() const;
	IconIterator GetIconsListEnd() const;
	const IconParam& GetIcon(unsigned int index) const;
	bool IsIconsListEmpty() const;

	// gets device icon URL
	// passing width & height equals to 0 function returns icon with standard parameters,
	// if it exists (mimetype = image/png).
	// otherwise icon is returned as specified, if any exists.
	// works on root device
	wstring GetDeviceIconURL(int iwidth, int iheight, int idepth) const;
	
	const Device* GetParentDevice() const;
	const Device* GetRootDevice() const;
	bool IsRoot() const;

	// with AddRef, don't forget to release interface when unused
	void GetInterface(IUPnPDevice** idev) const;

	// returns structure contains data which helps manipulate descr documents
	const DocAccessData* GetAccessData() const;

	// retrieves description document url from this UPnP device
	wstring GetDevDocAccessURL() const;
	// retrieves description document content from this UPnP device
	wstring GetDeviceDocument() const;

	// retrieves info about this UPnP device
	bool GetDeviceInfo(InfoData& data) const;

	// recursive process device object tree
	// see above instructions about IProcessDevice
	void EnumerateDevices(IProcessDevice* iproc, void* param, int procid) const;

private:
	// enumerates member devices of this UPnP device
	// and stores members in collection
	// calls EnumSrv
	bool EnumDev();
	// enumerates member services of this UPnP device
	// and stores collection in corresponding device object
	bool EnumSrv();

	// creates new device object and add one to collection
	// with AddRef()
	void AddDevice(IUPnPDevice* idev);
	// creates new service object and add one to collection
	// with AddRef()
	void AddService(IUPnPService* isrv);

	bool SetUDN();
	void GenerateFriendlyName();
	wstring GetDocURL() const;
	bool SetType();

	void RemoveAllDevices();
	void RemoveAllServices();

private:
	IUPnPDevice*	_idevice;		// COM interface of UPnP device
	const Device*	_parent;		// parent device object
	wstring			_udn;			// Unique Device Name of UPnP device
	wstring			_name;			// friendly name of UPnP device
	wstring			_type;			// type of UPnP device
	ServiceList		_services;		// list of service objects representing hosted UPnP services
	DeviceList		_devices;		// list of device objects representing hosted UPnP devices
	DocAccessData	_accessdata;	// helper data to maniplulate description documents
	IconList		_icons;			// list of icon resources for UPnP device
};


// ============== IUPnPDeviceFinder callback ============== //


// interface for communication with callback client
struct IFinderCallbackClient
{
	virtual void DeviceAdded(long findid, IUPnPDevice* idev) = 0;
	virtual void DeviceRemoved(long findid, const wstring& devname) = 0;
	virtual void SearchComplete(long findid) = 0;
	// for threads synchronization when adding and removing devices from collection
	virtual void Lock() = 0;
	virtual void UnLock() = 0;
};


// interface for communication with FindManager client
struct IFinderManagerClient
{
	virtual void OnStartFindDevice(long findid) = 0;
	virtual void OnStopFindDevice(long findid, bool iscancelled) = 0;
	virtual void OnAddDevice(long findid, const Device* dev, int devindex) = 0;
	virtual void OnRemoveDevice(long findid, const wstring& devudn, const wstring& friendlyname, int removedindex) = 0;
};


// IUPnPDeviceFinderCallback implementation
class DevFinderCallback : public IUPnPDeviceFinderCallback, public IEventInvoke
{
	friend class FindManager;

private: 
	IFinderCallbackClient*	_client;
	long					_refcount;

public:
	virtual ~DevFinderCallback() {}

	// IEventInvoke implementation
	virtual void InvokeClientEvent(const EventData* data);

	// IUnknown implementation
	virtual HRESULT _stdcall QueryInterface(const IID& riid, void** ppvObject);
	virtual unsigned long _stdcall AddRef();
	virtual unsigned long _stdcall Release();

	// IUPnPDeviceFinderCallback implementation
	virtual HRESULT __stdcall DeviceAdded(long findid, IUPnPDevice* idev);
	virtual HRESULT __stdcall DeviceRemoved(long findid, BSTR devudn);
	virtual HRESULT __stdcall SearchComplete(long findid);

private:
	explicit DevFinderCallback(IFinderCallbackClient* client);

	DevFinderCallback(const DevFinderCallback& srcobj);
	DevFinderCallback& operator= (const DevFinderCallback& srcobj);
};


// ============== FindManager class ============== //


// class for manage IUPnPFinder* object and devices collection.
// To manage external device's collection outside this class
// create your own class implementing IFinderCallbackClient interface
// then call FindManager::Init passing pointer to your own object.
// Else, pass pointer to client of type IFinderManagerClient
// to manage collection internally and notify client about events.

class FindManager : public IFinderCallbackClient, public IProcessDevice
{
public:
	FindManager();
	virtual ~FindManager();

	// devices collection will be managed internaly.
	// pass interface of client which will be notified about events related to collection
	bool Init(IFinderManagerClient* client, const wstring& devicetype = L"upnp:rootdevice");
	// external collection.
	// pass pointer to your own class implementing IFinderCallbackClient.
	bool Init(IFinderCallbackClient* client, const wstring& devicetype = L"upnp:rootdevice");

	// starts new search
	bool Start();
	// stops current search
	bool Stop();

	// access to devices collection
	// if collection is managed externally then throws exception
	const DeviceArray* GetCollection() const;
	DeviceArrayIterator GetCollectionBegin() const;
	DeviceArrayIterator GetCollectionEnd() const;
	const Device* GetDevice(unsigned int index) const;
	const Device* operator[] (unsigned int index) const;
	int GetCollectionCount() const;
	bool IsCollectionEmpty() const;

	// current search identifier
	long GetFindId();

	// when devices collection is managed externally in your own class
	// then you should manually add callback to services events using Service::SetCallbackClient.
	// this function is used only if FindManager manages devices collection.
	// adds callback to services and notify given client about events related to service.
	// if client will be set to null pointer then callback won't be added to the collection
	// and client won't be notified about events.
	void SetServiceEventClientPtr(IServiceCallbackClient* client);

	// IFinderCallbackClient implementation
	// calls EnterCriticalSection and LeaveCriticalSection
	void Lock();
	void UnLock();

	// standard UPnP devices types
	static const wchar_t* device_type[];
	static wstring GetRootDeviceType();
	static int TypesCount();

private:
	bool Init(const wstring& devicetype);

	// releases finder callback
	void ReleaseCallback();

	// IProcessDevice implementation
	// for assign callback's client to each service object in devices tree
	virtual void ProcessDevice(const Device* dev, void* param, int procid);

	// IFinderCallbackClient implementation
	virtual void DeviceAdded(long findid, IUPnPDevice* idev);
	virtual void DeviceRemoved(long findid, const wstring& devname);
	virtual void SearchComplete(long findid);

	void RemoveAllDevices();

private:
	DevFinderCallback*			_findercallback;		// IUPnPDeviceFinderCallback implementation
	IUPnPDeviceFinder*			_ifinder;				// IUPnPDeviceFinder interface
	DeviceArray					_devs;					// device's collection
	long						_finderhandle;			// IUPnPDeviceFinder find handle
	IFinderManagerClient*		_findermanagerclient;	// pointer to client receiving events related to changes
														// in devices collection managed internally by FindManager
	IFinderCallbackClient*		_findercallbackclient;	// pointer to client which manages devices collection
	IServiceCallbackClient*		_srveventclient;		// pointer to client receiving services events
	bool						_externalcollection;	// true if collection is managed externally

	CRITICAL_SECTION			_cs;					// for synchronize access to devices collection

	FindManager(const FindManager& srcobj);
	FindManager& operator= (const FindManager& srcobj);
};



// =====================
// UPnP helper functions
// =====================



// converts hresult of action's invokation to message string
wstring GetErrorMessage(HRESULT hr);

// converts description of state variable type to variant type
VARTYPE GetVariantType(const wstring& vtype);

// converts variant type to description of state variable type
wstring GetTypeDescr(VARTYPE vtype);

// checks for Internet Connection Sharing enabled connections
bool IsICSConnEnabled();

// checks system service state
bool CheckServiceState(const wstring& srvname, /*out*/srvstate& state);

// starts or stops ssdp service
bool ControlSSDPService(bool start);

// checks firewall port state
// 0 - error, 1 - enabled, -1 - disabled
int CheckFirewallPortState(long number, transport_protocol protocol);

// opens or close UPnP ports (2869 TCP, 1900 UDP)
bool ControlUPnPPorts(bool open);


}

#endif
