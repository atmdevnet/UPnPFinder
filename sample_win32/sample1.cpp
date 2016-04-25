// In this sample FindManager manages internal devices collection

#define _WIN32_DCOM
#define _CRTDBG_MAP_ALLOC

#include <crtdbg.h>
#include <iostream>
#include "upnpcplib.h"

#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

using namespace UPnPCpLib;
using namespace std;

// This class hosts FindManager object and uses its ability to manage
// devices collection, thus implements IFinderManagerClient interface
// to receive notifications from FindManager about changes in collection
// and start/stop events. To receive notifications about events
// from device's services this class implements IServiceCallbackClient.

class FinderClient : public IFinderManagerClient, public IServiceCallbackClient, public IProcessDevice
{
public:
    FinderClient()
        : _searching(false)
    {
        _fm = new FindManager();	// create new FindManager object
        // creates IUPnPDeviceFinderCallback object,
        // pointer to this class is passed, to manage devices internally
		// and receive notifications from FindManager
		_fm->Init(this);
        // sets receiver (this class) of services events
		_fm->SetServiceEventClientPtr(this);

        wcout << L"\nFinderClient was created successfully";
    }

    ~FinderClient()
    {
        if(IsSearching())
            StopSearch();

        // destroy FindManager object
		delete _fm;

        // when all objects were destroyed then
        // post WM_QUIT message to message loop, to finish it
        PostQuitMessage(0);

        wcout << L"\nFinderClient has been destroyed";
    }

    bool StartSearch()
    {
        // start finding devices
        // with default argument "upnp:rootdevice"
        return (_searching = _fm->Start());
    }

    bool StopSearch()
    {
        // stop finding devices
        return !(_searching = !_fm->Stop());
    }

    bool IsSearching() {return _searching;}

    void InvokeAction(unsigned int devindex, const wstring& actionName, const StrList& args)
    {
		_action = 0;

        // find service related to given action name
        // and if found then save pointer to requested action object.
        // see ProcessDevice function.
		const Device* dev = _fm->GetDevice(devindex);
        dev->EnumerateDevices(this, (void*)&actionName, 0);

		// if action found then invoke it
		if(_action != 0)
		{
			wcout << L"\nInvoke action: " << actionName << endl;

			wstring result;
			ArgsArray argsout;

			const_cast<Action*>(_action)->SetInArgs(args);
			_action->Invoke(argsout);

			for(ArgsArray::size_type i = 0; i < argsout.size(); ++i)
				result.append(argsout[i].second).append(L"\n");

			wcout << L"action result:" << endl;
			
			// write result to console
			wcout << result;
		}
		else
			wcout << L"\nAction not found" << endl;
    }

private:
	// IProcessDevice interface implementation.
    // This interface is implemented to use Device::EnumerateDevices function
    virtual void ProcessDevice(const Device* dev, void* param, int procid)
    {
		const wstring& actname = *(const wstring*)param;

		ServiceIterator srvi = dev->GetServiceListBegin();
		int srvcount = dev->GetServiceListCount();

		// enumerate member services of given device
		for(int i = 0; i < srvcount; ++i, ++srvi)
		{
			// check if current service contains requested action
			// if so, then save pointer.
			// btw, sorry for this "exceptional coding" style :)
			try
			{
				_action = &(*srvi)->GetAction(actname);
				break;
			}
			catch(...)
			{
			}
		}
    }

    // IFinderManagerClient interface implementation
    virtual void OnStartFindDevice(long findid)
    {
        // called by FindManager when finding was started successfully
        wcout << L"\nSearching started";
    }

    virtual void OnStopFindDevice(long findid, bool iscancelled)
    {
        // called by FindManager when finding was stopped successfully
        
		// display reason of calling
        wcout << L"\nSearching stopped because of " << (iscancelled ? L"cancel" : L"search complete");

        if(!iscancelled)
        {
            // when search is complete (not cancelled)
            // let's break loop and finish application
            wcout << L"\nSearching will be cancelled";
            this->StopSearch();
        }
    }

    virtual void OnAddDevice(long findid, const Device* dev, int devindex)
    {
        // called by FindManager when device object has been added to collection
        
		// display device name
        wcout << L"\nDevice added: " << dev->GetFriendlyName() ;

        // display passed device object properties
		InfoData devdata;
		if(dev->GetDeviceInfo(devdata))
		{
			for(InfoIterator ii = devdata.begin(); ii != devdata.end(); ++ii)
				wcout << endl << (*ii).first << L" = " << (*ii).second;
		}

		wcout << L"\nSearching is being continued";
    }

    virtual void OnRemoveDevice(long findid, const wstring& devudn, const wstring& friendlyname, int removedindex)
    {
        // called by FindManager when device object has been removed from collection
        
		// display removed device name
        wcout << L"\nDevice removed: " << friendlyname << L"\nSearching is being continued";
    }

    // IServiceCallbackClient interface implementation
    virtual void ServiceEventVariableChanged(const Service* srv, const wstring& varname, const wstring& varvalue)
    {
        // called by service callback when value of state variable has been changed

        // get pointer to parent device
        wstring devparent = srv->GetParentDevice().GetFriendlyName();

		// display device's name, service's id, name of state variable
		// and its current value
		wcout << L"\n==> Event fired:\n\tfrom service id: " << srv->GetServiceID() 
			<< L"\n\tparent device: " << devparent 
			<< L"\n\tsource state variable name: " << varname 
			<< L"\n\tcurrent state variable value: " << varvalue 
			<< L"\nSearching is being continued";
    }

    virtual void ServiceEventInstanceDied(const Service* srv)
    {
        // called by service callback when service is not responding
        wcout << L"\nService died: " << srv->GetServiceID()
            << "\nIt was member of device: " << srv->GetParentDevice().GetFriendlyName()
            << "\nSearching is being continued";
    }

private:
    bool			_searching;	// helps break message loop
    FindManager*	_fm;		// object of class from UPnPCpLib library
	const Action*	_action;	// requested action name
};


int main()
{
	int f = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	f |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(f);

    // initialize COM library.
	// in console application, COM library is initialized
	// to use single threaded concurrency model
    if(CoInitializeEx(0, COINIT_APARTMENTTHREADED) != S_OK)
    {
        CoUninitialize();
        return -1;
    }

    wcout << L"\nCOM library initialized";

    // initialize Winsock library
    WSADATA wdata;
    WSAStartup(MAKEWORD(2,2), &wdata);

    wcout << L"\nWinsock library initialized";
    wcout << L"\nCreating FinderClient object";

    // create FinderClient object which hosts FinderManager
    FinderClient* fclnt = new FinderClient();

    // start finding all root devices
    wcout << L"\nStart finding\n";

    if(fclnt->StartSearch())
    {
        // process messages
        MSG Message;
        while(GetMessage(&Message, 0, 0, 0))
        {
            DispatchMessage(&Message);      
            wcout << '.';

            // when FinderClient receives "search complete" notification
            // its function IsSearching returns false
            // and FinderClient will be destroyed and application finished
            if(!fclnt->IsSearching())
            {
				// let's execute some actions
				wstring name;
				StrList args;

				// action without input arguments
				name = L"GetExternalIPAddress";

				fclnt->InvokeAction(0, name, args);

				// it has one input argument of type ui2
				name = L"GetGenericPortMappingEntry";
				args.push_back(L"0"); // entry's index

				fclnt->InvokeAction(0, name, args);

                wcout << L"\nFinderClient finished work and will be destroyed\n";
                // in FinderClient destructor WM_QUIT message is posted
                // to break this loop
                delete fclnt;
				fclnt = 0;
            }
        }
    }

	delete fclnt;

    wcout << L"\nCleaning up Winsock library";
    // free Winsock library
    WSACleanup();

    wcout << L"\nCleaning up COM library";
    // free COM library
    CoUninitialize();

    wcout << L"\nExiting... bye";

    return 0;
}

