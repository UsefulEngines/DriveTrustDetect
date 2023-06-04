//**************************************************************************
//  2008 Microsoft Corporation.  For illustration purposes only.
//**************************************************************************

#pragma once

#include "DiskDrive.h"


#pragma pack(push,1)
struct	IDENTIFY_DEVICE_OUTDATA
{
	SENDCMDOUTPARAMS	sSendCmdOutParam;
	BYTE	pData[ATA_DISK_SECTOR_SIZE - 1];
};
#pragma	pack(pop)


interface IAtaInterface : public IBusInterface
{
	virtual bool ReadIdentifySector(_bstr_t &rbstrErrorInfo)
	{
		TRACE(L"IAtaInterface::ReadIdentifySector\n");
		CDiskDrive<IAtaInterface> *pDisk = dynamic_cast<CDiskDrive<IAtaInterface>*>(this);
		ASSERT(pDisk);
		ASSERT(pDisk->BytesPerSector() <= sizeof(TAtaDiskIdentifySector));

		DWORD dwReturnedLength   = 0;
		ATA_PASS_THROUGH_DIRECT aptd = { sizeof(aptd) };
		aptd.AtaFlags            = ATA_FLAGS_DATA_IN | ATA_FLAGS_DRDY_REQUIRED;
		aptd.DataBuffer          = (void*)&pDisk->IdentifySector()._sectorData;
		aptd.DataTransferLength  = pDisk->BytesPerSector();
		aptd.TimeOutValue        = 0x0f;

		IDEREGS& regs = (IDEREGS&)(aptd.CurrentTaskFile);
		regs.bCommandReg   = 0xEC;		// IDE_ATA_IDENTIFY, ATAPI_IDENTIFY_DEVICE
		regs.bDriveHeadReg = 0x40;

		if (!pDisk->DeviceIo(
			IOCTL_ATA_PASS_THROUGH_DIRECT,
			&aptd,
			sizeof(aptd),
			&aptd,
			sizeof(aptd),
			&dwReturnedLength,
			NULL) )
		{
			if ( 0x01 & regs.bCommandReg )			// error bit set.
			{
				rbstrErrorInfo = ::BuildMessage(L"IAtaInterface::ReadIdentifySector : Error=0x%02X : "
					L"NoMedia=%02X : "
					L"Abort=%02X : "
					L"MediaChangeRequest=%02X : "
					L"DeviceNotFound=%02X : "
					L"MediaChanged=%02x : "
					L"Uncorr=%02X : "
					L"IntrCRC=%02X\n", 
					regs.bFeaturesReg,
					regs.bFeaturesReg & 0x02,
					regs.bFeaturesReg & 0x04,
					regs.bFeaturesReg & 0x08,
					regs.bFeaturesReg & 0x10,
					regs.bFeaturesReg & 0x20,
					regs.bFeaturesReg & 0x40,
					regs.bFeaturesReg & 0x80);
				TRACE((const wchar_t*)rbstrErrorInfo);
			}
			else
			{
				::TranslateErrorCode(::GetLastError(), rbstrErrorInfo);
				rbstrErrorInfo = ::BuildMessage(L"IAtaInterface::ReadIdentifySector : %ws\n", (const wchar_t*)rbstrErrorInfo);
			}
			return false;
		}

		// TODO : Deeper success checking? 
		if (dwReturnedLength < sizeof(aptd))  
			return false;						
		return true;
	}

	virtual bool Send(_bstr_t &rbstrErrorInfo, const BYTE *pbyBuffer, unsigned nSizeBuffer)
	{
		TRACE(L"IAtaInterface::Send\n");
		CDiskDrive<IAtaInterface> *pDisk = dynamic_cast<CDiskDrive<IAtaInterface>*>(this);
		ASSERT(pDisk);
		ASSERT((pbyBuffer != NULL) && (nSizeBuffer > 0));

		DWORD bytesMoved = 0;
		ATA_PASS_THROUGH_DIRECT aptd = { sizeof(aptd) };
		aptd.DataTransferLength = nSizeBuffer;
		aptd.DataBuffer         = (void*)pbyBuffer;
		aptd.TimeOutValue       = 0x0F;

		IDEREGS& regs = (IDEREGS&)(aptd.CurrentTaskFile);
		::ZeroMemory( &regs, sizeof(regs) );
		regs.bFeaturesReg       = 0xF0;										// Protocol ID Vendor Unique, see the T13 specs
		regs.bSectorCountReg    = (UCHAR)(nSizeBuffer / pDisk->BytesPerSector());     // ATA disk sector size is 512 bytes of data  
		regs.bDriveHeadReg      = 0x40;

		aptd.AtaFlags           = ATA_FLAGS_DATA_OUT | ATA_FLAGS_DRDY_REQUIRED;
		regs.bCommandReg        = 0x5E;  // Trusted Send, PIO data-out : see the T13 specs (ATA6 and ATA8)

		if (!pDisk->DeviceIo(
			IOCTL_ATA_PASS_THROUGH_DIRECT,
			&aptd,
			sizeof(aptd),
			&aptd,
			sizeof(aptd),
			&bytesMoved,
			NULL) )
		{
			if ( 0x01 & regs.bCommandReg ) // error bit set.
			{
				rbstrErrorInfo = ::BuildMessage(L"IAtaInterface::Send : Error=0x%02X : "
					L"NoMedia=%02X : "
					L"Abort=%02X : "
					L"MediaChangeRequest=%02X : "
					L"DeviceNotFound=%02X : "
					L"MediaChanged=%02x : "
					L"Uncorr=%02X : "
					L"IntrCRC=%02X\n", 
					regs.bFeaturesReg,
					regs.bFeaturesReg & 0x02,
					regs.bFeaturesReg & 0x04,
					regs.bFeaturesReg & 0x08,
					regs.bFeaturesReg & 0x10,
					regs.bFeaturesReg & 0x20,
					regs.bFeaturesReg & 0x40,
					regs.bFeaturesReg & 0x80);
				TRACE((const wchar_t*)rbstrErrorInfo);
			}
			else
			{
				::TranslateErrorCode(::GetLastError(), rbstrErrorInfo);
				rbstrErrorInfo = ::BuildMessage(L"IAtaInterface::Send : %ws\n", (const wchar_t*)rbstrErrorInfo);
			}
			return false;
		}
		// TODO : Deeper success checking? 
		return true;
	}

	virtual bool Receive(_bstr_t &rbstrErrorInfo, const BYTE *pbyBuffer, unsigned nSizeBuffer)
	{
		TRACE(L"IAtaInterface::Receive\n");
		CDiskDrive<IAtaInterface> *pDisk = dynamic_cast<CDiskDrive<IAtaInterface>*>(this);
		ASSERT(pDisk);
		ASSERT((pbyBuffer != NULL) && (nSizeBuffer > 0));

		DWORD bytesMoved = 0;
		ATA_PASS_THROUGH_DIRECT aptd = { sizeof(aptd) };
		aptd.DataTransferLength = nSizeBuffer;
		aptd.DataBuffer         = (void*)pbyBuffer;
		aptd.TimeOutValue       = 0x0F;

		IDEREGS& regs = (IDEREGS&)(aptd.CurrentTaskFile);
		::ZeroMemory( &regs, sizeof(regs) );
		regs.bFeaturesReg       = 0xF0;  // Protocol ID Vendor Unique
		regs.bSectorCountReg    = (UCHAR)(nSizeBuffer / pDisk->BytesPerSector());     // ATA disk sector size is 512 bytes of data 
		regs.bDriveHeadReg      = 0x40;

		aptd.AtaFlags           = ATA_FLAGS_DATA_IN | ATA_FLAGS_DRDY_REQUIRED;
		regs.bCommandReg        = 0x5C;  // Trusted Receive, PIO data-in

		if (!pDisk->DeviceIo(
			IOCTL_ATA_PASS_THROUGH_DIRECT,
			&aptd,
			sizeof(aptd),
			&aptd,
			sizeof(aptd),
			&bytesMoved,
			NULL) )
		{
			if ( 0x01 & regs.bCommandReg ) // error bit set.
			{
				rbstrErrorInfo = ::BuildMessage(L"IAtaInterface::Receive : Error=0x%02X : "
					L"NoMedia=%02X : "
					L"Abort=%02X : "
					L"MediaChangeRequest=%02X : "
					L"DeviceNotFound=%02X : "
					L"MediaChanged=%02x : "
					L"Uncorr=%02X : "
					L"IntrCRC=%02X\n", 
					regs.bFeaturesReg,
					regs.bFeaturesReg & 0x02,
					regs.bFeaturesReg & 0x04,
					regs.bFeaturesReg & 0x08,
					regs.bFeaturesReg & 0x10,
					regs.bFeaturesReg & 0x20,
					regs.bFeaturesReg & 0x40,
					regs.bFeaturesReg & 0x80);
				TRACE((const wchar_t*)rbstrErrorInfo);
			}
			else
			{
				::TranslateErrorCode(::GetLastError(), rbstrErrorInfo);
				rbstrErrorInfo = ::BuildMessage(L"IAtaInterface::Receive : %ws\n", (const wchar_t*)rbstrErrorInfo);
			}
			return false;
		}
		// TODO : Deeper success checking? 
		return true;
	}

};  // IAtaInterface


