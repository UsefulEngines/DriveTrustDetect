
#pragma once


//  The ATA/ATAPI 'IdentifySector' is defined within the "T13 ATA/ATAPI" drive interface
//  specification.
//
//		see:	http://www.t13.org
//				http://www.ata-atapi.com/hist.htm
//				The "EnumDisk" sample at http://support.microsoft.com/kb/264203.
//				The "ScsiQueryDevice" sample at http://blogs.msdn.com/adioltean/articles/344588.aspx.
//

#define ATA_DISK_SECTOR_SIZE  512		// Size, in bytes, of the ATA disk Sector (also the Identify Sector)

#pragma pack(push,1)
//
//	ATA Disk Identify Sector 
//  
//  The fields of this struct should match the T13 ATA/ATAPI specification.
//
typedef struct TAtaDiskIdentifySector
{
	unsigned __int16	wGeneralConfiguration;		//0	  15 bit:0=ATA  
	unsigned __int16	wObsolute1;					//1
	unsigned __int16	wSpecificConfiguration;		//2
	unsigned __int16	wObsolute2;					//3
	unsigned __int16	wRetired1[2];				//4-5
	unsigned __int16	wObsolute3;					//6
	unsigned __int32	ulReservedForCompactFlash;	//7-8
	unsigned __int16	wRetired2;					//9
	char				pszSerialNumber[20];		//10-19		
	unsigned __int32	ulRetired3;					//20-21
	unsigned __int16	wObsolute4;					//22
	char				pszFirmwareRev[8];			//23-26		
	char				pszModelNumber[40];			//27-46		
	unsigned __int16	wMaxNumPerInterupt;			//47
	unsigned __int16	wReserved1;					//48
	unsigned __int16	wCapabilities1;				//49		
	unsigned __int16	wCapabilities2;				//50
	unsigned __int32	ulObsolute5;				//51-52
	unsigned __int16	wField88and7063;			//53		
	unsigned __int16	wObsolute6[5];				//54-58
	unsigned __int16	wMultSectorStuff;			//59
	unsigned __int32	ulTotalAddressableSectors;	//60-61
	unsigned __int16	wObsolute7;					//62
	unsigned __int16	wMultiWordDMA;				//63		
	unsigned __int16	wPIOMode;					//64
	unsigned __int16	wMinMultiwordDMACycleTime;	//65
	unsigned __int16	wRecommendedMultiwordDMACycleTime;	//66
	unsigned __int16	wMinPIOCycleTimewoFlowCtrl;	//67
	unsigned __int16	wMinPIOCycleTimeWithFlowCtrl;	//68
	unsigned __int16	wReserved2[6];				//69-74
	unsigned __int16	wQueueDepth;				//75
	unsigned __int16	wReserved3[4];				//76-79
	unsigned __int16	wMajorVersion;				//80		
	unsigned __int16	wMinorVersion;				//81
	unsigned __int16	wCommandSetSupported1;		//82		
	unsigned __int16	wCommandSetSupported2;		//83		
	unsigned __int16	wCommandSetSupported3;		//84		
	unsigned __int16	wCommandSetEnable1;			//85	
	unsigned __int16	wCommandSetEnable2;			//86		
	unsigned __int16	wCommandSetDefault;			//87
	unsigned __int16	wUltraDMAMode;				//88		
	unsigned __int16	wTimeReqForSecurityErase;	//89
	unsigned __int16	wTimeReqForEnhancedSecure;	//90
	unsigned __int16	wCurrentPowerManagement;	//91
	unsigned __int16	wMasterPasswordRevision;	//92
	unsigned __int16	wHardwareResetResult;		//93
	unsigned __int16	wAcoustricmanagement;		//94
	unsigned __int16	wStreamMinRequestSize;		//95
	unsigned __int16	wStreamingTimeDMA;			//96
	unsigned __int16	wStreamingAccessLatency;	//97
	unsigned __int32	ulStreamingPerformance;		//98-99
	unsigned __int16	pwMaxUserLBA[4];			//100-103
	unsigned __int16	wStremingTimePIO;			//104
	unsigned __int16	wReserved4;					//105
	unsigned __int16	wSectorSize;				//106
	unsigned __int16	wInterSeekDelay;			//107
	unsigned __int16	wIEEEOUI;					//108
	unsigned __int16	wUniqueID3;					//109
	unsigned __int16	wUniqueID2;					//110
	unsigned __int16	wUniqueID1;					//111
	unsigned __int16	wReserved5[4];				//112-115
	unsigned __int16	wReserved6;					//116
	unsigned __int32	ulWordsPerLogicalSector;	//117-118
	unsigned __int16	wReserved7[8];				//119-126
	unsigned __int16	wRemovableMediaStatus;		//127
	unsigned __int16	wSecurityStatus;			//128
	unsigned __int16	pwVendorSpecific[31];		//129-159
	unsigned __int16	wCFAPowerMode1;				//160
	unsigned __int16	wReserved8[15];				//161-175
	char				pszCurrentMediaSerialNo[60];//176-205
	unsigned __int16	wReserved9[49];				//206-254
	unsigned __int16	wIntegrityWord;				//255		
} TAtaDiskIdentifySector;
//
#pragma	pack(pop)

static const unsigned int nSizeTAtaDiskIdentifySector = sizeof(TAtaDiskIdentifySector);
static const char *pszEmptyString = "\0";
static unsigned char cDestBuffer[nSizeTAtaDiskIdentifySector];
 

// The TIdentifySector struct is used to hold the disk drive Identify Sector content.
//
typedef struct TIdentifySector
{
	TAtaDiskIdentifySector	_sectorData;	

	TIdentifySector(void) 
	{
		ASSERT((sizeof(TAtaDiskIdentifySector)) == ATA_DISK_SECTOR_SIZE);
		_sectorData.wGeneralConfiguration = 0;
		return; 
	}

	TIdentifySector(TIdentifySector &rSector) 
	{
		::memcpy_s((void*)&_sectorData, sizeof(TAtaDiskIdentifySector), (void*)&rSector._sectorData, sizeof(TAtaDiskIdentifySector));
	}

	TIdentifySector &operator=(TIdentifySector &rSector)
	{
		::memcpy_s((void*)&_sectorData, sizeof(TAtaDiskIdentifySector), (void*)&rSector._sectorData, sizeof(TAtaDiskIdentifySector));
		return *this;
	}

	inline void Initialize(void)
	{
		::ZeroMemory((void*)&_sectorData, sizeof(TAtaDiskIdentifySector));
	}

	inline bool IsSectorDataAvailable(void) { return (_sectorData.wGeneralConfiguration > 0); }

	inline const char* GetModel(void)
	{
		return(GetByteSwapField((const unsigned char*)&_sectorData.pszModelNumber[0], sizeof(_sectorData.pszModelNumber))); 
	}

	const char* GetFirmware(void)
	{
		return(GetByteSwapField((const unsigned char*)&_sectorData.pszFirmwareRev[0], sizeof(_sectorData.pszFirmwareRev))); 
	}

	const char* GetSerialNo(void)
	{
		return(GetByteSwapField((const unsigned char*)&_sectorData.pszSerialNumber[0], sizeof(_sectorData.pszSerialNumber))); 
	}

	const char* GetVendorID(void)
	{
		// TODO : build a table of vendor strings and return that based upon model analysis.
		if (IsSeagateModel())
			return("Seagate");
		else
			return(":-)");
	}

	bool IsSeagateModel(void)
	{
		if (_sectorData.wGeneralConfiguration > 0)
		{
			// Is this a Seagate drive (i.e. "ST")?  Note that the pszModelNumber field will be byte-swapped (i.e. "TS").
			return((::memcmp("TS", &_sectorData.pszModelNumber[0], 2)) == 0);
		}
		return false;
	}

	bool IsAtaPassthruCapable(void)
	{
		if (_sectorData.wGeneralConfiguration > 0)
		{
			// Non-zero values in words 76 and 79...?
			return (_sectorData.wReserved3[0] || _sectorData.wReserved3[3]);
		}
		return false;
	}

	bool IsDriveTrustCapable(void)
	{
		if (_sectorData.wGeneralConfiguration > 0)
		{
			// Identify Sector word 150 is word 21 of the pwVendorSpecific array.
			return (((_sectorData.pwVendorSpecific[21] & 0x10) > 0) &&
					((_sectorData.pwVendorSpecific[21] & 0x1000) > 0));
		}
		return false;
	}

  private:  
	const char *GetByteSwapField(const unsigned char *pBytes, unsigned short uSize)
	{
		ASSERT(pBytes != NULL);
		ASSERT((uSize > 0) && (uSize <= sizeof(_sectorData.pszCurrentMediaSerialNo)));	
		ASSERT((pBytes >= (unsigned char*)&_sectorData) && (pBytes < ((unsigned char*)(&_sectorData + nSizeTAtaDiskIdentifySector))));

		// try to avoid some buffer overrun potential
		if ((!pBytes) || (!IsSectorDataAvailable()) || (uSize > sizeof(_sectorData.pszCurrentMediaSerialNo)))
			return(pszEmptyString);

		// first copy specified field to the working cDestBuffer
		::ZeroMemory((void*)&cDestBuffer[0], sizeof(cDestBuffer));
		if (::memcpy_s(&cDestBuffer[0], sizeof(cDestBuffer), pBytes, uSize))
			return(pszEmptyString);   
		
		// next, swap the bytes within the cDestBuffer
		SwapBytes((unsigned char *)&cDestBuffer[0], uSize);
		cDestBuffer[uSize - 1] = '\0';

		// remove any prepended spaces	// TODO : move this code to a separate inline member method.
		unsigned char *pFront = cDestBuffer;
		while ((::isspace(*pFront)) && (*pFront != '\0'))
			pFront++;

		// remove any appended spaces	// TODO : move this code to a separate inline member method.
		unsigned char *pBack = &cDestBuffer[uSize - 2];
		while ((::isspace(*pBack)) && (pBack > pFront))
		{
			*pBack = '\0';
			pBack--;
		}
		return((const char *)pFront);
	}

	void SwapBytes(unsigned char *pBytes, unsigned short uSize)
	{
		unsigned short	i;
		unsigned char	c;

		for(i = 0; i < uSize; i += 2)
		{
			c = pBytes[i];
			pBytes[i] = pBytes[i + 1];
			pBytes[i+1] = c;
		}
	}
} TIdentifySector;


