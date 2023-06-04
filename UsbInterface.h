//**************************************************************************
//  2008 Microsoft Corporation.  For illustration purposes only.
//**************************************************************************

#pragma once

#include "DiskDrive.h"


//  SATA or ATA disk drive on a SCSI bus...
// 
//  Note that an external USB disk drive appears to the OS as a SCSI device.  This presents a 
//  problem in that we need to send ATA commands (not SCSI commands) to the drive.  Hence, we
//  must embed ATA control codes (i.e. "CDB settings") within the SCSI device IOCTL transactions
//  which the disk drive will interpret appropriately.  See the ATA6 and ATA8 interface specifications
//  at http://www.t13.org and the SCSI specification at http://www.t10.org.  Also see the "SPTI" 
//  sample within the Windows Device Driver Kit.  Also note that the ability to accomplish 
//  ATA pass-thru via SCSI pass-thru is dependent upon the capability of the USB bridge chipset.
//  This capability varies with USB device manufacturer.  
//
//		see:	http://www.t10.org
//				http://www.answers.com/topic/scsi-request-sense-command
//				http://www.seagate.com/support/kb/disc/scsi_sense_keys.html
//				http://t10.org/lists/1spc-lst.htm
//				http://members.aol.com/plscsi/cdbcomplete.html
//
#define SPT_SENSE_LENGTH		32	   // not used herein...  value from spti DDK sample.	
#define SPT_SENSE_MAX_LENGTH  0xFF	   // value used herein...  

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER 
{
    SCSI_PASS_THROUGH_DIRECT	sptd;
    ULONG						Filler;      // realign buffer to double word boundary
    UCHAR						ucSenseBuf[SPT_SENSE_MAX_LENGTH];					
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

// Command Descriptor Block (CDB) constants.
//
#define CDB6GENERIC_LENGTH                   6
#define CDB10GENERIC_LENGTH                  10


// The following structure is representative of SCSI sense info upon a successful SCSI 
// operation with a resultant status code as defined 
// here (http://www.answers.com/topic/scsi-status-code).
// We map this struct onto the sense buffer beginning at offset 8, that is:
//
//	  ATAReturnDescriptor *pSenseInfo = reinterpret_cast<ATAReturnDescriptor*>(&sptdwb.ucSenseBuf[8]);
//
#pragma pack(push,1)
typedef struct _ATAReturnDescriptor
{
   UCHAR DescriptorCode;               // 09h
   UCHAR AdditionalDescriptorLength;   // 0Ch
   UCHAR Extend;
   UCHAR Error;
   UCHAR SectorCount8to15;		// e.g. number of sectors read... 
   UCHAR SectorCount0to7;		// ""				
   UCHAR LBALow8to15;
   UCHAR LBALow0to7;
   UCHAR LBAMid8to15;
   UCHAR LBAMid0to7;
   UCHAR LBAHigh8to15;
   UCHAR LBAHigh0to7;
   UCHAR Device;
   UCHAR Status;				// corresponds to the ATA IDEREGS 'features' reg upon disk response.
} ATAReturnDescriptor;
#pragma	pack(pop)


interface IUsbInterface : public IBusInterface
{
	virtual bool ReadIdentifySector(_bstr_t &rbstrErrorInfo)
	{
		TRACE(L"IUsbInterface::ReadIdentifySector\n");
		CDiskDrive<IUsbInterface> *pDisk = dynamic_cast<CDiskDrive<IUsbInterface>*>(this);
		ASSERT(pDisk);
		ASSERT(pDisk->BytesPerSector() <= sizeof(TAtaDiskIdentifySector));

		// An assumption is that we actually have a SATA drive within a USB enclosure which thus
		// appears to be a SCSI device.  This function uses SCSI pass through to obtain the ATA
		// IDENTIFY DEVICE sector.  If this were really a SCSI disk device, then we would use
		// STORAGE_PROPERTY_QUERY and IOCTL_STORAGE_QUERY_PROPERTY instead to return a
		// STORAGE_DEVICE_DESCRIPTOR instead of an IDENTIFY DEVICE sector.  

		// Complications include varying levels of support for embedded ATA commands in the SCSI 
		// pass through functionality of differing USB bridge chipsets.  This function will be
		// tested with the Oxford and Initio bridge chipsets.  The Oxford bridge is used with the
		// Seagate Go external drive and the Initio bridge is used with the Maxtor OneTouch brand.

		SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER	sptdwb;  
		DWORD	dwReturnedLength = 0;
		ATAReturnDescriptor *pSenseInfo = NULL;

		::ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
		sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
		sptdwb.sptd.ScsiStatus = 0xFF; 
		sptdwb.sptd.PathId = 0;
		sptdwb.sptd.TargetId = (unsigned char)pDisk->SCSITargetId();
		sptdwb.sptd.Lun = (unsigned char)pDisk->SCSILogicalUnit();
		sptdwb.sptd.CdbLength = CDB10GENERIC_LENGTH;
		sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
		sptdwb.sptd.SenseInfoLength = sizeof(sptdwb.ucSenseBuf);;
		sptdwb.sptd.DataTransferLength = pDisk->BytesPerSector();
		sptdwb.sptd.TimeOutValue = 0x0f;
		sptdwb.sptd.DataBuffer = (void*)&(pDisk->IdentifySector()._sectorData);
		sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);

		// TODO : Get this translated according to specs...  It's currently only reverse engineered using the debugger.
		// TODO : Test with various USB bridge chipsets...  Tested with FreeAgent Go (i.e. Oxford).
		sptdwb.sptd.Cdb[0] = 0xA1;	
		sptdwb.sptd.Cdb[1] = 0x08;                        
		sptdwb.sptd.Cdb[2] = 0x2A;                        
		sptdwb.sptd.Cdb[4] = 0x01;                        
		sptdwb.sptd.Cdb[9] = 0xEC;                        

		if (!pDisk->DeviceIo(
			IOCTL_SCSI_PASS_THROUGH_DIRECT, 
			&sptdwb,
			sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),
			&sptdwb,
			sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),
			&dwReturnedLength,
			NULL))
		{
			::TranslateErrorCode(::GetLastError(), rbstrErrorInfo);
			rbstrErrorInfo = ::BuildMessage(L"IUsbInterface::ReadIdentifySector : %ws\n", (const wchar_t*)rbstrErrorInfo);
			return false;		
		}

		// Success or failure at the ATA protocol level:  http://www.answers.com/SCSI+Sense+Code
		if ((sptdwb.sptd.ScsiStatus <= 0x04) &&
			(sptdwb.ucSenseBuf[0] == 0x72))
		{
			// The Oxford and Initio USB bridge chipset appears to return 0x72 upon success...  TODO : verify with any other chipsets.
			// pSenseInfo = reinterpret_cast<ATAReturnDescriptor*>(&sptdwb.ucSenseBuf[8]);
			// ASSERT(pSenseInfo->SectorCount0to7 == 1);
			// TODO : deeper success analysis here?  Interpret sense codes?  Probably best to just leave status determination at a higher level (e.g. APDU response code).
			return true;
		}
		else  
		{
			pSenseInfo = reinterpret_cast<ATAReturnDescriptor*>(&sptdwb.ucSenseBuf[8]);
			if (pSenseInfo->Status > 0x00)
			{
				// The ATAReturnDescriptor::Status field corresponds to the ATA "Features" register upon failed response
				rbstrErrorInfo = ::BuildMessage(L"Drive status flags : "
					L"NoMedia=%02X : "
					L"Abort=%02X : "
					L"MediaChangeRequest=%02X : "
					L"DeviceNotFound=%02X : "
					L"MediaChanged=%02x : "
					L"Uncorr=%02X : "
					L"IntrCRC=%02X", 
					pSenseInfo->Status & 0x02,
					pSenseInfo->Status & 0x04,
					pSenseInfo->Status & 0x08,
					pSenseInfo->Status & 0x10,
					pSenseInfo->Status & 0x20,
					pSenseInfo->Status & 0x40,
					pSenseInfo->Status & 0x80);
			}
			rbstrErrorInfo = ::BuildMessage(L"IUsbInterface::ReadIdentifySector : operation failed.  ScsiStatus=0x%02X  ucSenseBuf[0]=0x%02X.  %ws\n", 
											sptdwb.sptd.ScsiStatus, sptdwb.ucSenseBuf[0], (const wchar_t*)rbstrErrorInfo);
			return false;   
		}
		//return true;
	}

	virtual bool Send(_bstr_t &rbstrErrorInfo, const BYTE *pbyBuffer, unsigned nSizeBuffer)
	{
		TRACE(L"IUsbInterface::Send\n");
		CDiskDrive<IUsbInterface> *pDisk = dynamic_cast<CDiskDrive<IUsbInterface>*>(this);
		ASSERT(pDisk);
		ASSERT((pbyBuffer != NULL) && (nSizeBuffer > 0));

		if ((!pbyBuffer) || (nSizeBuffer == 0))
		{
			rbstrErrorInfo += ::BuildMessage(L"E_INVALIDARG : %ws : %ws ", __FILE__, __LINE__);
			return false;
		}

		SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER	sptdwb;  
		DWORD	dwReturnedLength = 0;
		ATAReturnDescriptor *pSenseInfo = NULL;

		::ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
		sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
		sptdwb.sptd.ScsiStatus = 0xFF; 
		sptdwb.sptd.PathId = 0;
		sptdwb.sptd.TargetId = (unsigned char)pDisk->SCSITargetId();
		sptdwb.sptd.Lun = (unsigned char)pDisk->SCSILogicalUnit();
		sptdwb.sptd.CdbLength = CDB10GENERIC_LENGTH;
		sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
		sptdwb.sptd.SenseInfoLength = sizeof(sptdwb.ucSenseBuf);  
		sptdwb.sptd.DataTransferLength = nSizeBuffer;
		sptdwb.sptd.TimeOutValue = 0x0f;
		sptdwb.sptd.DataBuffer = (void*)pbyBuffer;
		sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);

		// TODO : Get this translated according to specs... Currently reverse engineered using debugger. 
		// TODO : Test with various USB bridge chipsets...  Tested with FreeAgent Go drive.
		sptdwb.sptd.Cdb[0] = 0xA1;	
		sptdwb.sptd.Cdb[1] = 0x0A;                        
		sptdwb.sptd.Cdb[2] = 0x22;
		sptdwb.sptd.Cdb[3] = 0xF0;
		sptdwb.sptd.Cdb[4] = 0x01;                        
		sptdwb.sptd.Cdb[9] = 0x5E;                        

		if (!pDisk->DeviceIo(
			IOCTL_SCSI_PASS_THROUGH_DIRECT, 
			&sptdwb,
			sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),
			&sptdwb,
			sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),
			&dwReturnedLength,
			NULL))
		{
			// TODO : deeper error analysis here.
			::TranslateErrorCode(::GetLastError(), rbstrErrorInfo);
			rbstrErrorInfo = ::BuildMessage(L"IUsbInterface::Send : %ws\n", (const wchar_t*)rbstrErrorInfo);
			return false;		
		}

		// success or failure at the ATA protocol level:  http://www.answers.com/SCSI+Sense+Code
		if ((sptdwb.sptd.ScsiStatus <= 0x04) &&
			(sptdwb.ucSenseBuf[0] == 0x72))
		{
			// The Oxford and Initio USB bridge chipset appears to return 0x72 upon success...  TODO : verify with any other chipsets.
			// pSenseInfo = reinterpret_cast<ATAReturnDescriptor*>(&sptdwb.ucSenseBuf[8]);
			// ASSERT(pSenseInfo->Status == 0x50);   // Oxford chipset returns 0x50
			// ASSERT(pSenseInfo->Status == 0x50);   // Initio 1605 chipset return 0x50
			// TODO : deeper success analysis here?  Interpret sense codes?  Probably best to just leave status determination to a higher level (e.g. APDU response code).
			return true;
		}
		else
		{
			pSenseInfo = reinterpret_cast<ATAReturnDescriptor*>(&sptdwb.ucSenseBuf[8]);
			if (pSenseInfo->Status > 0x00)
			{
				// The ATAReturnDescriptor::Status field corresponds to the ATA "Features" register upon failed response
				rbstrErrorInfo = ::BuildMessage(L"Drive status flags : "
					L"NoMedia=%02X : "
					L"Abort=%02X : "
					L"MediaChangeRequest=%02X : "
					L"DeviceNotFound=%02X : "
					L"MediaChanged=%02x : "
					L"Uncorr=%02X : "
					L"IntrCRC=%02X", 
					pSenseInfo->Status & 0x02,
					pSenseInfo->Status & 0x04,
					pSenseInfo->Status & 0x08,
					pSenseInfo->Status & 0x10,
					pSenseInfo->Status & 0x20,
					pSenseInfo->Status & 0x40,
					pSenseInfo->Status & 0x80);
			}
			rbstrErrorInfo = ::BuildMessage(L"IUsbInterface::ReadIdentifySector : operation failed.  ScsiStatus=0x%02X  ucSenseBuf[0]=0x%02X.  %ws\n", 
											sptdwb.sptd.ScsiStatus, sptdwb.ucSenseBuf[0], (const wchar_t*)rbstrErrorInfo);
			return false;   
		}
		//return true;
	}

	virtual bool Receive(_bstr_t &rbstrErrorInfo, const BYTE *pbyBuffer, unsigned nSizeBuffer)
	{
		TRACE(L"IUsbInterface::Receive\n");
		CDiskDrive<IUsbInterface> *pDisk = dynamic_cast<CDiskDrive<IUsbInterface>*>(this);
		ASSERT(pDisk);
		ASSERT((pbyBuffer != NULL) && (nSizeBuffer > 0));

		if ((!pbyBuffer) || (nSizeBuffer == 0))
		{
			rbstrErrorInfo += ::BuildMessage(L"E_INVALIDARG : %ws : %ws ", __FILE__, __LINE__);
			return false;
		}

		SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER	sptdwb;  
		DWORD	dwReturnedLength = 0;
		ATAReturnDescriptor *pSenseInfo = NULL;

		::ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
		sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
		sptdwb.sptd.ScsiStatus = 0xFF; 
		sptdwb.sptd.PathId = 0;
		sptdwb.sptd.TargetId = (unsigned char)pDisk->SCSITargetId();
		sptdwb.sptd.Lun = (unsigned char)pDisk->SCSILogicalUnit();
		sptdwb.sptd.CdbLength = CDB10GENERIC_LENGTH;
		sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
		sptdwb.sptd.SenseInfoLength = SPT_SENSE_LENGTH;
		sptdwb.sptd.DataTransferLength = nSizeBuffer;
		sptdwb.sptd.TimeOutValue = 0x0f;
		sptdwb.sptd.DataBuffer = (void*)pbyBuffer;
		sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);

		// TODO : Get this translated according to ATA specs...  It's currently only reverse engineered using the debugger.
		// TODO : Test with various USB bridge chipsets...  Tested with FreeAgent Go (i.e. Oxford) and Initio 1605 (i.e. Maxtor 1-touch).
		sptdwb.sptd.Cdb[0] = 0xA1;	
		sptdwb.sptd.Cdb[1] = 0x08;                        
		sptdwb.sptd.Cdb[2] = 0x2A;
		sptdwb.sptd.Cdb[3] = 0xF0;
		sptdwb.sptd.Cdb[4] = 0x01;                        
		sptdwb.sptd.Cdb[9] = 0x5C;                        

		if (!pDisk->DeviceIo(
			IOCTL_SCSI_PASS_THROUGH_DIRECT, 
			&sptdwb,
			sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),
			&sptdwb,
			sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER),
			&dwReturnedLength,
			NULL))
		{
			// TODO : deeper error analysis here.
			::TranslateErrorCode(::GetLastError(), rbstrErrorInfo);
			rbstrErrorInfo = ::BuildMessage(L"IUsbInterface::Receive : %ws\n", (const wchar_t*)rbstrErrorInfo);
			return false;		
		}

		// success or failure at the ATA protocol level:  http://www.answers.com/SCSI+Sense+Code
		if ((sptdwb.sptd.ScsiStatus <= 0x04) &&
			(sptdwb.ucSenseBuf[0] == 0x72))
		{
			// The Oxford USB bridge chipset appears to return 0x72 upon success...  TODO : verify with other chipsets.
			// pSenseInfo = reinterpret_cast<ATAReturnDescriptor*>(&sptdwb.ucSenseBuf[8]);
			// ASSERT((pSenseInfo->SectorCount0to7 >= 1) || (pSenseInfo->SectorCount8to15 >= 1));
			// ASSERT(pSenseInfo->Status == 0x58);    // Oxford returns 0x58
			// ASSERT(pSenseInfo->Status == 0x50);    // Initio 1605 returns 0x50
			// TODO : deeper success analysis here?  Interpret sense codes?  Probably best to just leave status determination to a higher level (e.g. APDU response code).
			return true;
		}
		else
		{
			pSenseInfo = reinterpret_cast<ATAReturnDescriptor*>(&sptdwb.ucSenseBuf[8]);
			if (pSenseInfo->Status > 0x00)
			{
				// The ATAReturnDescriptor::Status field corresponds to the ATA "Features" register upon failed response
				rbstrErrorInfo = ::BuildMessage(L"Drive status flags : "
					L"NoMedia=%02X : "
					L"Abort=%02X : "
					L"MediaChangeRequest=%02X : "
					L"DeviceNotFound=%02X : "
					L"MediaChanged=%02x : "
					L"Uncorr=%02X : "
					L"IntrCRC=%02X", 
					pSenseInfo->Status & 0x02,
					pSenseInfo->Status & 0x04,
					pSenseInfo->Status & 0x08,
					pSenseInfo->Status & 0x10,
					pSenseInfo->Status & 0x20,
					pSenseInfo->Status & 0x40,
					pSenseInfo->Status & 0x80);
			}
			rbstrErrorInfo = ::BuildMessage(L"IUsbInterface::ReadIdentifySector : operation failed.  ScsiStatus=0x%02X  ucSenseBuf[0]=0x%02X.  %ws\n", 
											sptdwb.sptd.ScsiStatus, sptdwb.ucSenseBuf[0], (const wchar_t*)rbstrErrorInfo);
			return false;   
		}
		//return true;
	}
};

