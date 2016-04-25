// In this sample devices collection is managed outside of FindManager

#define _WIN32_DCOM

#include <iostream>
#include "upnpcplib.h"

using namespace UPnPCpLib;
using namespace std;


// This class hosts FindManager object and DON'T uses its ability to manage
// devices collection. Instead, this class on its own is managing the collection.
// In this case FindManager is used only for finding devices.
// Thus this class implements IFinderCallbackClient interface
// to receive notifications from UPnP framework about found devices.
// To receive notifications about events from device's services
// this class implements IServiceCallbackClient.

class FinderHost : public IFinderCallbackClient, public IServiceCallbackClient, public IProcessDevice
{
public:
    FinderHost()
        : _searching(false)
		, _finish(false)
    {
        // create IUPnPDeviceFinderCallback object
        // and pass pointer to FinderHost which will be managing devices collection
        _fm = new FindManager();
        _fm->Init(this);

        wcout << L"FinderHost was created successfully" << endl;
    }

    ~FinderHost()
    {
        if(IsSearching())
            StopSearch();

        // remove devices collection
		RemoveAllDevices();

        // destroy FindManager object
		delete _fm;

        // when all objects were destroyed then
        // post WM_QUIT message to message loop, to finish it
        //PostQuitMessage(0);

        wcout << L"\nFinderHost has been destroyed" << endl;
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

	void SetFinish() {_finish = true;}
	bool IsFinished() {return _finish;}

	// IFinderCallbackClient implementation
	// in single threading model it's unneccessary to synchronize access
	// to devices collection, thus function bodies are empty
	void Lock() {}
	void UnLock() {}

    void InvokeAction(int devindex, const wstring& actionName, const StrList& args)
    {
        _action = 0;

        // find service related to given action name
        // and if found then save pointer to requested action object.
        // see ProcessDevice function.
		const Device* dev = _devs[devindex];
        dev->EnumerateDevices(this, (void*)&actionName, PID_FIND_ACTION);

		// if action found then invoke it
		if(_action != 0)
		{
			wcout << L"\nInvoke action: " << actionName << flush;

			wstring result;
			ArgsArray argsout;

			const_cast<Action*>(_action)->SetInArgs(args);
			_action->Invoke(argsout);

			for(ArgsArray::size_type i = 0; i < argsout.size(); ++i)
				result.append(argsout[i].second).append(L"\n");

			wcout << L"\naction result:" << endl;
			
			// write result to console
			wcout << result << endl;
		}
		else
			wcout << L"\nAction not found" << endl;
    }

private:

	// identifiers used in ProcessDevice
	static const int PID_ADD_SERVICE_CALLBACK = 0;
	static const int PID_FIND_ACTION = 1;

    // IProcessDevice interface implementation.
    // This interface is implemented for use Device::EnumerateDevices function
    virtual void ProcessDevice(const Device* dev, void* param, int procid)
    {
        switch(procid)
        {
            // for add pointer to client of services callbacks
        case PID_ADD_SERVICE_CALLBACK:
            {
				ServiceIterator srvi = dev->GetServiceListBegin();
				int srvcount = dev->GetServiceListCount();

				for(int i = 0; i < srvcount; ++i, ++srvi)
					const_cast<Service*>(*srvi)->SetCallbackClient(this);
            }
            break;

            // for find service containing given action name
        case PID_FIND_ACTION:
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
            break;
        }
    }

    // IFinderCallbackClient interface implementation

    virtual void DeviceAdded(long findid, IUPnPDevice* idev)
    {
        // called by UPnP framework when the new device was found

		// create new root Device object and build its structure
		Device* dev = new Device(idev);

		// add callback for services events
		dev->EnumerateDevices(this, 0, PID_ADD_SERVICE_CALLBACK);

		// add Device root object to collection
		_devs.push_back(dev);

		wcout << L"\nNew device added: " << dev->GetFriendlyName() << endl;

		// display info about added device
		InfoData devdata;
        if(dev->GetDeviceInfo(devdata))
        {
			for(InfoIterator ii = devdata.begin(); ii != devdata.end(); ++ii)
				wcout << endl << (*ii).first << L" = " << (*ii).second;
        }

		wcout << L"\nSearching is being continued" << flush;
    }

    virtual void DeviceRemoved(long findid, const wstring& devname)
    {
        // called by UPnP framework when the device was removed from collection

		for(DeviceArray::size_type i = 0; i < _devs.size(); ++i)
		{
			if(devname == _devs[i]->GetUDN())
			{
				wstring friendlyname = _devs[i]->GetFriendlyName();

				// remove passed device from devices collection
				delete _devs[i];
				_devs.erase(_devs.begin() + i);

				wcout	<< L"\nDevice was removed: " << friendlyname 
						<< L"\nSearching is being continued" << flush;

				break;
			}
		}
    }

    virtual void SearchComplete(long findid)
    {
        // called by UPnP framework when finding was completed successfully

        wcout << L"\nSearch is complete" << endl;

        // when search is complete (not cancelled)
        // let's break loop and finish application
        wcout << L"Searching will be cancelled" << endl;
        this->StopSearch();
    }

    // IServiceCallbackClient interface implementation
    virtual void ServiceEventVariableChanged(const Service* srv, const wstring& varname, const wstring& varvalue)
    {
        // called by service callback when value of state variable has been changed

        // get parent device name
		wstring devparent = srv->GetParentDevice().GetFriendlyName();
        // display device's name, service's id, name of state variable
        // and its current value
		wcout << L"\n==> Event fired:\n\tfrom service id: " << srv->GetServiceID() 
            << L"\n\tparent device: " << devparent
            << L"\n\tsource state variable name: " << varname
            << L"\n\tcurrent state variable value: " << varvalue << endl;

		if(_searching)
			wcout << L"Searching is being continued" << flush;

		// when all notifications are received then
        // post WM_QUIT message to message loop, to finish it
        if(_finish)
			PostQuitMessage(0);
    }
    virtual void ServiceEventInstanceDied(const Service* srv)
    {
        // called by service callback when service is not responding
        wcout << L"\nService died: " << srv->GetServiceID()
            << L"\nIt was member of device: " << srv->GetParentDevice().GetFriendlyName()
            << L"\nSearching is being continued" << flush;
    }

	void RemoveAllDevices()
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

private:
    bool			_searching;	// helps break message loop
	bool			_finish;	// helps break message loop
    FindManager*	_fm;		// object of class from UPnPCpLib library
    DeviceArray		_devs;		// devices collection
	const Action*	_action;	// requested action name
};



int _tmain(int argc, _TCHAR* argv[])
{
    // initialize COM library.
	// in console application, COM library is initialized
	// to use single threaded concurrency model
    if(CoInitializeEx(0, COINIT_APARTMENTTHREADED) != S_OK)
    {
        CoUninitialize();
        return -1;
    }

    wcout << L"COM library initialized" << endl;

    // initialize Winsock library
    WSADATA wdata;
    WSAStartup(MAKEWORD(2, 2), &wdata);

    wcout <<	L"Winsock library initialized\n"
				L"Creating FinderHost object" << endl;

    // create FinderHost object which hosts FinderManager
    FinderHost* fhost = new FinderHost();

    // start finding all root devices
    wcout << L"Start finding" << flush;

    if(fhost->StartSearch())
    {
        // process messages
        MSG Message;
        while(GetMessage(&Message, NULL, 0, 0))
        {
            DispatchMessage(&Message);      
			wcout << '.' << flush;

            // when FinderHost receives "search complete" notification
            // its function IsSearching returns false
            // and FinderHost will be destroyed and application finished
            if(!fhost->IsSearching() && !fhost->IsFinished())
            {
                wcout << L"\nSearching has been cancelled successfully" << endl;

				// let's execute some actions
				wstring name;
				StrList args;

				// this action doesn't return any output arguments
				// but fires event from state variable PortMappingNumberOfEntries
				name = L"AddPortMapping";
				args.push_back(L"any");				// remote host, type: string
				args.push_back(L"11200");			// external port, type: ui2
				args.push_back(L"UDP");				// protocol, type: string
				args.push_back(L"11200");			// internal port, type: ui2
				args.push_back(L"192.168.0.10");	// internal client, type: string
				args.push_back(L"true");			// enabled, type: boolean
				args.push_back(L"add port mapping example"); // description, type: string
				args.push_back(L"0");				// lease duration (0 = permanent), type: ui4

				fhost->InvokeAction(0, name, args);

				// this action also doesn't return any output arguments
				// but fires event from state variable PortMappingNumberOfEntries
				name = L"DeletePortMapping";
				args.clear();
				args.push_back(L"any");			// remote host, type: string
				args.push_back(L"11200");		// external port, type: ui2
				args.push_back(L"UDP");			// protocol, type: string

				fhost->InvokeAction(0, name, args);

                wcout << L"\nFinderHost finished work and will be destroyed" << endl;
                // in FinderHost ServiceEventVariableChanged WM_QUIT message is posted
                // to break this loop
				fhost->SetFinish();
                //delete fhost;
				//fhost = 0;
            }
        }
    }

	delete fhost;

    wcout << L"Cleaning up Winsock library" << endl;
    // free Winsock library
    WSACleanup();

    wcout << L"Cleaning up COM library" << endl;
    // free COM library
    CoUninitialize();

    wcout << L"Exiting... bye" << endl;

    return 0;
}

