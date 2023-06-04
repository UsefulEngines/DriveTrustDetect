//**************************************************************************
//  2008 Microsoft Corporation.  For illustration purposes only.
//**************************************************************************

#pragma once

#include "stdafx.h"
#include "AtaIdentifySector.h"

interface IBusInterface;
template <typename T> class CDiskDrive;
typedef CDiskDrive<IBusInterface> *pCDiskDrive;
typedef std::vector<pCDiskDrive> TListDiskDrives; 

//  The IBusInterface type encapsulates the methods associated with reading and writing to the 
//  disk drive via varying bus interfaces (e.g. ATA, USB, SCSI, etc.).  Additionally, and especially
//  with external USB drives, the USB bridge chipset model will introduce further complexities.
//  Use this interface to isolate those complexities.  See AtaInterface.h and UsbInterface.h.

interface IBusInterface
{
	virtual bool ReadIdentifySector(_bstr_t &rbstrErrorInfo) = 0;
	virtual bool Send(_bstr_t &rbstrErrorInfo, const BYTE *pbyBuffer, unsigned nSizeBuffer) = 0;
	virtual bool Receive(_bstr_t &rbstrErrorInfo, const BYTE *pbyBuffer, unsigned nSizeBuffer) = 0;
};


//  The CDiskDrive class illustrates an encapsulation of information and functionality
//  related to a WMI descriptive disk drive object.  The bus type for accessing the drive is 
//  encapsulated within the IBusInterface derived type.  We are only interested
//  in SATA or ATA type drives possessing an ATA Identify Sector.  The SCSI related parameters
//  are only used to interact with external USB drives which present a SCSI DeviceIoControl pass-thru 
//  interface.  See AtaIdentifySector.h.
//  TODO : There is an operation timeout value associated with the DeviceIoControl API.  It's is
//  herein hard-coded at 15 seconds (i.e. 0x0F).  Need to expose this as a parameter for modification.

template <typename IBusInterfaceType> 
class CDiskDrive : public IBusInterfaceType
{
	friend				IBusInterfaceType;
  private:	
	_bstr_t				_bstrName;				// WMI Win32_DiskDrive : DeviceID
	_bstr_t				_bstrInterfaceType;		// WMI Win32_DiskDrive : InterfaceType (SCSI, hdc, IDE, USB, 1394)
	unsigned int		_nBytesPerSector;		// WMI Win32_DiskDrive : BytesPerSector (e.g. 512)
	unsigned short		_nSCSIBus;				// WMI Win32_DiskDrive : SCSIBus
	unsigned short		_nSCSILogicalUnit;		// WMI Win32_DiskDrive : SCSILogicalUnit
	unsigned short		_nSCSIPort;				// WMI Win32_DiskDrive : SCSIPort
	unsigned short		_nSCSITargetId;			// WMI Win32_DiskDrive : SCSITargetId
	HANDLE				_hDevice;				// Windows HANDLE to the disk device				
	TIdentifySector		_sIdentifySector;		// The disk "Identify Sector" content
	_bstr_t				_bstrModel;				// Derived from the "Identify Sector"
	_bstr_t				_bstrFirmware;			// ""
	_bstr_t				_bstrSerialNo;			// ""
	_bstr_t				_bstrVendorID;			// ""
	CRITICAL_SECTION	_critSection;			// Synchronize threads to ensure Send/Receive pairs are atomic.

  protected:
	inline bool Send(_bstr_t &rbstrErrorInfo, const BYTE *pbyBuffer, unsigned short nLength)
	{
		return(dynamic_cast<IBusInterfaceType*>(this)->Send(rbstrErrorInfo, pbyBuffer, nLength));
	}

	inline bool Receive(_bstr_t &rbstrErrorInfo, const BYTE *pbyBuffer, unsigned short nLength)
	{
		return(dynamic_cast<IBusInterfaceType*>(this)->Receive(rbstrErrorInfo, pbyBuffer, nLength));
	}

	BOOL DeviceIo(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
	{
		TRACE(L"CDiskDrive::DeviceIo\n");

		// This function might be used to accomplish thread-level synchronization of device IO calls to the kernel disk device object.
		// The IBusInterface derived types will call back into this function from their Read/Send/Receive interface member functions.
		// However, one required aspect of APDU synchronization is enforcing send/receive pairs (i.e. not interrupting an atomic send/receive pair).
		// Consequently, the 'Transmit' function below is the place to enforce this.

		ASSERT(HandleIsValid()); 	
		BOOL bres;

		bres = ::DeviceIoControl(Handle(),
			dwIoControlCode, 
			lpInBuffer,
			nInBufferSize,
			lpOutBuffer,
			nOutBufferSize,
			lpBytesReturned,
			lpOverlapped);

		return bres;
	}

  public:
	bool QueryIdentifySector(_bstr_t &rbstrErrorInfo)
	{
		TRACE(L"CDiskDrive::QueryIdentifySector\n");
		bool bres = true;
		ASSERT(HandleIsValid()); 

		if (!HandleIsValid())
		{
			rbstrErrorInfo = L"QueryIdentifySector : Invalid device handle.";
			return false;
		}
		_sIdentifySector.Initialize();
		
		// ACCOMPLISH THREAD SYNCHRONIZATION AROUND THIS SEND/RECEIVE TRANSACTION!
		::EnterCriticalSection(&_critSection); 
		bres = dynamic_cast<IBusInterfaceType*>(this)->ReadIdentifySector(rbstrErrorInfo);
		::LeaveCriticalSection(&_critSection);

		if (bres == false)
		{
			TranslateErrorCode(::GetLastError(), rbstrErrorInfo);
			rbstrErrorInfo = ::BuildMessage(L"Error : %ws : %ws : Failed to read the disk 'Identify Sector'. : %ws", 
				(const wchar_t*)_bstrName,
				(const wchar_t*)_bstrInterfaceType,
				(const wchar_t*)rbstrErrorInfo);
		}
		return bres;
	}

	// Accessors
	inline bool HandleIsValid(void) 
		{ return (_hDevice != INVALID_HANDLE_VALUE); }
	
	inline const HANDLE &Handle(void) 
		{ return _hDevice; }

	inline const _bstr_t &Name(void) 
		{ return _bstrName; }

	inline const _bstr_t &InterfaceType(void) 
		{ return _bstrInterfaceType; }

	inline const int BytesPerSector(void) 
		{ return _nBytesPerSector; }

	inline const int ScsiBus(void) 
		{ return _nSCSIBus; }

	inline const short SCSILogicalUnit(void) 
		{ return _nSCSILogicalUnit; }

	inline const short SCSIPort(void) 
		{ return _nSCSIPort; }

	inline const short SCSITargetId(void) 
		{ return _nSCSITargetId; }

	inline const TIdentifySector &IdentifySector(void) 
		{ return _sIdentifySector; }

	inline bool IsAtaPassthruCapable(void) 
		{ return _sIdentifySector.IsAtaPassthruCapable(); }

	inline const _bstr_t &Model(void) 
	{ 
		if (_bstrModel.length() == 0)
			_bstrModel = _sIdentifySector.GetModel();
		return _bstrModel; 
	}
	inline const _bstr_t &Firmware(void)
	{ 
		if (_bstrFirmware.length() == 0)
			_bstrFirmware = _sIdentifySector.GetFirmware();
		return _bstrFirmware; 
	}
	inline const _bstr_t &SerialNo(void)
	{ 
		if (_bstrSerialNo.length() == 0)
			_bstrSerialNo = _sIdentifySector.GetSerialNo();
		return _bstrSerialNo; 
	}
	inline const _bstr_t &VendorID(void)
	{ 
		if (_bstrVendorID.length() == 0)
			_bstrVendorID = _sIdentifySector.GetVendorID();
		return _bstrVendorID; 
	}

	// Constructors and destructor
	CDiskDrive()
	{
		_hDevice = INVALID_HANDLE_VALUE;
		_nBytesPerSector = IDENTIFY_BUFFER_SIZE;
		_nSCSIBus = 0;
		_nSCSILogicalUnit = 0;
		_nSCSIPort = 0;
		_nSCSITargetId = 0;
		if (!::InitializeCriticalSectionAndSpinCount(&_critSection, 0x80000400))
			throw ::BuildMessage(L"Initialize critical section : %ws : %ws", __FILE__, __LINE__);
	}

	CDiskDrive(CDiskDrive &rInfo) : _bstrName(rInfo._bstrName), 
		_bstrInterfaceType(rInfo._bstrInterfaceType),
		_nDriveIndex(rInfo._nDriveIndex),
		_nBytesPerSector(rInfo._nBytesPerSector),
		_nSCSIBus(rInfo._nSCSIBus),
		_nSCSILogicalUnit(rInfo._nSCSILogicalUnit),
		_nSCSIPort(rInfo._nSCSIPort),
		_nSCSITargetId(rInfo._nSCSITargetId)
	{
		if (::DuplicateHandle(::GetCurrentProcess(), 
			rInfo._hDevice, 
			::GetCurrentProcess(),
			&this->_hDevice, 
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS) == 0)
			_hDevice = INVALID_HANDLE_VALUE;
		_sIdentifySector = rInfo._sIdentifySector;
		ASSERT(_nBytesPerSector <= (sizeof(_sIdentifySector._sectorData)));
		if (!::InitializeCriticalSectionAndSpinCount(&_critSection, 0x80000400))
			throw ::BuildMessage(L"Initialize critical section : %ws : %ws", __FILE__, __LINE__);
	}

	CDiskDrive(CDiskDrive *pInfo)
	{
		if (pInfo)
		{
			_bstrName = pInfo->_bstrName;
			_bstrInterfaceType = pInfo->_bstrInterfaceType;
			if (::DuplicateHandle(::GetCurrentProcess(), 
				pInfo->_hDevice, 
				::GetCurrentProcess(),
				&this->_hDevice, 
				0,
				FALSE,
				DUPLICATE_SAME_ACCESS) == 0)
				_hDevice = INVALID_HANDLE_VALUE;
			this->_sIdentifySector = pInfo->_sIdentifySector;
			this->_nBytesPerSector = pInfo->_nBytesPerSector;
			ASSERT(this->_nBytesPerSector <= (sizeof(this->_sIdentifySector._sectorData)));
			this->_nSCSIBus = pInfo->_nSCSIBus;
			this->_nSCSILogicalUnit = pInfo->_nSCSILogicalUnit;
			this->_nSCSIPort = pInfo->_nSCSIPort;
			this->_nSCSITargetId = pInfo->_nSCSITargetId;
			if (!::InitializeCriticalSectionAndSpinCount(&_critSection, 0x80000400))
				throw ::BuildMessage(L"Initialize critical section : %ws : %ws", __FILE__, __LINE__);
		}
		else
			throw ::BuildMessage(L"E_POINTER : %ws : %ws", __FILE__, __LINE__);
	}

	CDiskDrive &operator=(CDiskDrive &rInfo)
	{
		this->_bstrName = rInfo._bstrName;
		this->_bstrInterfaceType = rInfo._bstrInterfaceType;
		if (::DuplicateHandle(::GetCurrentProcess(), 
			rInfo._hDevice, 
			::GetCurrentProcess(),
			&this->_hDevice, 
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS) == 0)
			this->_hDevice = INVALID_HANDLE_VALUE;
		this->_sIdentifySector = rInfo._sIdentifySector;
		this->_sIdentifySector = rInfo._sIdentifySector;
		this->_nBytesPerSector = rInfo._nBytesPerSector;
		ASSERT(this->_nBytesPerSector <= (sizeof(this->_sIdentifySector._sectorData)));
		this->_nSCSIBus = rInfo._nSCSIBus;
		this->_nSCSILogicalUnit = rInfo._nSCSILogicalUnit;
		this->_nSCSIPort = rInfo._nSCSIPort;
		this->_nSCSITargetId = rInfo._nSCSITargetId;
		if (!::InitializeCriticalSectionAndSpinCount(&_critSection, 0x80000400))
			throw ::BuildMessage(L"Initialize critical section : %ws : %ws", __FILE__, __LINE__);
		return *this;
	}

	CDiskDrive(BSTR bstrName, 
		BSTR bstrInterfaceType, 
		HANDLE hDevice,
		unsigned int nBytesPerSector,
		unsigned int nSCSIBus,
		unsigned short nSCSILogicalUnit,
		unsigned short nSCSIPort,
		unsigned short nSCSITargetId) 
	{
		// This constructor only used within GetDiskDriveDevices() herein.
		// Assume it is safe to ignore handle closure potential.
		_bstrName = bstrName; 
		_bstrInterfaceType = bstrInterfaceType;
		_hDevice = hDevice; 
		_nBytesPerSector = nBytesPerSector;
		ASSERT(_nBytesPerSector <= (sizeof(_sIdentifySector._sectorData)));
		_nSCSIBus = (unsigned short)nSCSIBus;
		_nSCSILogicalUnit = nSCSILogicalUnit;
		_nSCSIPort = nSCSIPort;
		_nSCSITargetId = nSCSITargetId;
		if (!::InitializeCriticalSectionAndSpinCount(&_critSection, 0x80000400))
			throw ::BuildMessage(L"Initialize critical section : %ws : %ws", __FILE__, __LINE__);
	}

	~CDiskDrive()
	{
		if (_hDevice != INVALID_HANDLE_VALUE)
			::CloseHandle(_hDevice);	
		::DeleteCriticalSection(&_critSection);
	}
};   // CDiskDrive




interface IUnsupportedInterface : public IBusInterface
{
	virtual bool ReadIdentifySector(_bstr_t &rbstrErrorInfo)
	{
		return Unsupported(rbstrErrorInfo);
	}
	virtual bool Send(_bstr_t &rbstrErrorInfo, const BYTE *, unsigned )    
	{
		return Unsupported(rbstrErrorInfo);
	}
	virtual bool Receive(_bstr_t &rbstrErrorInfo, const BYTE *, unsigned )
	{
		return Unsupported(rbstrErrorInfo);
	}
	inline bool Unsupported(_bstr_t &rbstrErrorInfo)
	{
		TRACE(L"IUnsupportedInterface\n");
		rbstrErrorInfo = L"Unsupported bus interface type.";
		return false;
	}
};   // IUnsupportedInterface


