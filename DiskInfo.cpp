//************************************************************************
//  File name: DiskInfo.cpp
//
//  Description: 
//  This sample enumerates connected storage devices on Windows.  we use
//  WMI to obtain the list of attached disk drive devices then use
//  DeviceIoControl to obtain the disk's "Identify Sector" data.  
//
//  Comments: 
//		1.  WMI is used to enumerate attached storage devices.
//			Alternative implementations may leverage the
//			Win32 SetupAPI library instead (e.g. 'SetupDiGetClassDevs') or
//			perhaps reading storage device information directly from the
//			registry.  
//
//		3.  Portions of this sample require further work.  Search for the string
//			"TODO" herein.
//
//  History:    Date       Author						Comment
//              10/2008    Phil@UsefulEngines.com       keyboard monkey
//
//  2008 Microsoft Corporation.  For illustration purposes only.
//**************************************************************************

#include "stdafx.h"
#include "DiskDrive.h"
#include "UsbInterface.h"
#include "AtaInterface.h"

#pragma comment(lib, "wbemuuid.lib")	// link with this lib for the WMI API's.

HRESULT GetDiskDriveDevices(TListDiskDrives &list);

int _tmain(int argc, _TCHAR* argv[])
{
	int							nret = 0;
	//wchar_t						wch;
	HRESULT						hr = S_OK;
	_bstr_t						bstrOnFailure;

	if (ValidOptions(argc, argv) == false)
		return nret;

	hr = ::CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		DisplayErrorMessage(L"COM runtime initialization failed.");
		return(hr);
	}

	try
	{
		TListDiskDrives				listDiskDrives;
		TListDiskDrives::iterator	iterDiskDrives;
		pCDiskDrive					pDisk = NULL;

		DisplayMessage(L"\nEnumerating disk drive devices...\n");

		// Enumerate disk drive devices
		hr = GetDiskDriveDevices(listDiskDrives);
		if (FAILED(hr))
			throw hr;

		for (iterDiskDrives = listDiskDrives.begin(); iterDiskDrives != listDiskDrives.end(); iterDiskDrives++)
		{
			pDisk = (pCDiskDrive)*iterDiskDrives;

			// Read and display each disk's "Identify Sector" information.
			if (pDisk->QueryIdentifySector(bstrOnFailure) == true)
			{
				DisplayMessage(L"\n%ws" 
								L"\n\tInterface= %ws" 
								L"\n\tModel= %ws"
								L"\n\tVendor= %ws"
								L"\n\tSerialNo= %ws"
								L"\n\tFirmware= %ws" 
								L"\n\tATA Passthru Capable= %ws\n", 
								(const wchar_t*)pDisk->Name(),
								(const wchar_t*)pDisk->InterfaceType(),
								(const wchar_t*)pDisk->Model(),
								(const wchar_t*)pDisk->VendorID(),
								(const wchar_t*)pDisk->SerialNo(),
								(const wchar_t*)pDisk->Firmware(),
								(pDisk->IsAtaPassthruCapable() ? L"Yes" : L"No"));
			}
			else 
				DisplayMessage((const wchar_t*)bstrOnFailure);
		}
		//DisplayMessage(L"\n\nPress any key to continue...\n");
		//wch = _getwch();
	}
	catch (wchar_t *pszMessage)
	{
		DisplayErrorMessage(pszMessage);
		nret = E_UNEXPECTED;
    }
	catch (const HRESULT& hres)
	{
		DisplayErrorMessage(hres);
		nret = hres;
	}
	catch (_com_error& Err)
	{
		DisplayErrorMessage(Err);
		nret = Err.Error();
	}
	catch (...)
	{
		nret = GetLastError();
		TranslateErrorCode(nret, bstrOnFailure);
		DisplayMessage(L"Exception : %08X : %ws\n", nret, (const wchar_t*)bstrOnFailure);
	}
		
	::CoUninitialize();		// ensure COM is exited
	return nret;
}


HRESULT GetDiskDriveDevices(TListDiskDrives &rList)
{
	TRACE(L"GetDiskDriveDevices\n");
    HRESULT hres = S_OK;
    IWbemLocator *pLoc = NULL;
    IWbemServices *pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;

	// See the WMI SDK documentation.

	// If this code is used within an existing established security context, then the CoInitializeSecurity call can be commented-out.
	// You'll know this case during testing because the code will fail at runtime here.
	hres =  CoInitializeSecurity(
        NULL, 
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
        );
                      
    if (FAILED(hres))
		goto CleanUp;
    
    // Obtain the initial locator to WMI 
	hres = CoCreateInstance(
        CLSID_WbemLocator,             
        0, 
        CLSCTX_INPROC_SERVER, 
        IID_IWbemLocator, (LPVOID *) &pLoc);
 
    if (FAILED(hres))
		goto CleanUp;
	ASSERT(pLoc != NULL);

    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = pLoc->ConnectServer(
         _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
         NULL,                    // User name. NULL = current user
         NULL,                    // User password. NULL = current
         0,                       // Locale. NULL indicates current
         NULL,                    // Security flags.
         0,                       // Authority (e.g. Kerberos)
         0,                       // Context object 
         &pSvc                    // pointer to IWbemServices proxy
         );
    
    if (FAILED(hres))
		::_com_issue_errorex(hres, pLoc, __uuidof(pLoc));
	ASSERT(pSvc != NULL);

    // Set security levels on the proxy
	hres = CoSetProxyBlanket(
       pSvc,                        // Indicates the proxy to set
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
       NULL,                        // Server principal name 
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
       NULL,                        // client identity
       EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres))
		goto CleanUp;

    // Use the IWbemServices pointer to make requests of WMI
    // Get a list of the attached Disk Drives
    hres = pSvc->ExecQuery(bstr_t("WQL"), 
						   bstr_t("SELECT * FROM Win32_DiskDrive"),
						   WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
						   NULL,
						   &pEnumerator);
    
    if (FAILED(hres))
		goto CleanUp;

    // Get the data from the query above
    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    VARIANT vtDeviceID, vtInterfaceType, vtBytesPerSector, vtSCSIBus, vtSCSILogicalUnit, vtSCSIPort, vtSCSITargetId;
	HANDLE hDevice;
	pCDiskDrive pInfo = NULL;
   
    while (pEnumerator)
    {
        pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if ((0 == uReturn) || (!pclsObj))
            break;
		ASSERT(pclsObj);

		::VariantInit(&vtDeviceID);
		::VariantInit(&vtInterfaceType);
		::VariantInit(&vtBytesPerSector);
		::VariantInit(&vtSCSIBus);
		::VariantInit(&vtSCSILogicalUnit);
		::VariantInit(&vtSCSIPort);
		::VariantInit(&vtSCSITargetId);
		hDevice = INVALID_HANDLE_VALUE;
		pInfo = NULL;

        // Get the value of the DeviceID property
        pclsObj->Get(L"DeviceID", 0, &vtDeviceID, 0, 0);
		pclsObj->Get(L"InterfaceType", 0, &vtInterfaceType, 0, 0);
		pclsObj->Get(L"BytesPerSector", 0, &vtBytesPerSector, 0, 0);
		pclsObj->Get(L"SCSIBus", 0, &vtSCSIBus, 0, 0);
		pclsObj->Get(L"SCSILogicalUnit", 0, &vtSCSILogicalUnit, 0, 0);
		pclsObj->Get(L"SCSIPort", 0, &vtSCSIPort, 0, 0);
		pclsObj->Get(L"SCSITargetId", 0, &vtSCSITargetId, 0, 0);
		
		TRACE(L"DeviceID=%ws \t InterfaceType=%ws\n", vtDeviceID.bstrVal, vtInterfaceType.bstrVal);
		// TODO: Obtain any other WMI device info from the pclsObj

		// Open a HANDLE to the device object.
		hDevice = CreateFile(vtDeviceID.bstrVal,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_NO_BUFFERING,
                               NULL);

		// Cache the new CDiskDrive object.
		if (hDevice != INVALID_HANDLE_VALUE)
		{
			if (_bstr_t("USB") == _bstr_t(vtInterfaceType.bstrVal))
				pInfo = reinterpret_cast<pCDiskDrive>(new CDiskDrive<IUsbInterface>(
						vtDeviceID.bstrVal, 
						vtInterfaceType.bstrVal, 
						hDevice,
						vtBytesPerSector.uintVal,
						vtSCSIBus.uintVal,
						vtSCSILogicalUnit.uiVal,
						vtSCSIPort.uiVal,
						vtSCSITargetId.uiVal));
			else if (_bstr_t("IDE") == _bstr_t(vtInterfaceType.bstrVal))							
				pInfo = reinterpret_cast<pCDiskDrive>(new CDiskDrive<IAtaInterface>(
						vtDeviceID.bstrVal, 
						vtInterfaceType.bstrVal, 
						hDevice,
						vtBytesPerSector.uintVal,
						vtSCSIBus.uintVal,
						vtSCSILogicalUnit.uiVal,
						vtSCSIPort.uiVal,
						vtSCSITargetId.uiVal));
			else
				pInfo = reinterpret_cast<pCDiskDrive>(new CDiskDrive<IUnsupportedInterface>(
						vtDeviceID.bstrVal, 
						vtInterfaceType.bstrVal, 
						hDevice,
						vtBytesPerSector.uintVal,
						vtSCSIBus.uintVal,
						vtSCSILogicalUnit.uiVal,
						vtSCSIPort.uiVal,
						vtSCSITargetId.uiVal));
			if (pInfo)
				rList.push_back(pInfo);
			else
				hres = E_OUTOFMEMORY;
		}

		::VariantClear(&vtDeviceID);
		::VariantClear(&vtInterfaceType);
		::VariantClear(&vtBytesPerSector);
		::VariantClear(&vtSCSIBus);
		::VariantClear(&vtSCSILogicalUnit);
		::VariantClear(&vtSCSIPort);
		::VariantClear(&vtSCSITargetId);
 		pclsObj->Release();
		pclsObj = NULL;
	}

CleanUp:   
	if (pSvc)
		pSvc->Release();
	if (pLoc)
		pLoc->Release();
	if (pEnumerator)
		pEnumerator->Release();

    return hres;   
}

