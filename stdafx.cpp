//**************************************************************************
//  2008 Microsoft Corporation.  For illustration purposes only.
//**************************************************************************

#include "stdafx.h"


// *********************************************************************************
// Utility Functions 
//

void DisplayUsage(wchar_t *progname)
{
	DisplayMessage(	L"Usage:\n\n  %ws [-g] [-?] \n\n"	
					L"  -? Display this message\n"						
					L"\t(note:  no arguments executes with program defaults)", 
					progname);
}


bool ValidOptions(int argc, _TCHAR *argv[]) 
{
	bool bRet = true;

	for( int i = 1; i < argc; i++ ) 
	{
		if( (argv[i][0] == L'-') || (argv[i][0] == L'/') ) 
		{
			switch( tolower(argv[i][1]) ) 
			{
			case L'?':
				DisplayUsage(argv[0]);
				return(false);

			// TODO : add new command line options here.

			default:	// unrecognized option
				DisplayUsage(argv[0]);
				return(false);
			}
		}
	}   
	return(bRet);
}


// TODO : Add concurrency protection around these data structures if needed.
static wchar_t msg[8192];			
static wchar_t description[2048];
static wchar_t source[1048];
static wchar_t iface[1048];
static const wchar_t *szFormat1 = L"Error: %08X %s\nDescription: %s\nFrom: %s\nInterface: %s\n";
static const wchar_t *szFormat2 = L"Error: %08X %s\nDescription: %s";


void DisplayErrorMessage(const HRESULT& hr)
{
	_ftprintf_s(stderr,  L"\nERROR : %ws\n", _com_error(hr).ErrorMessage());
	return;
}


void DisplayErrorMessage(const wchar_t *pszError)
{
	if (pszError != NULL)
	{
		_ftprintf_s(stderr,  L"\nERROR : %ws\n", pszError);
	}
	return;
}


void DisplayErrorMessage(_com_error &e)
{
	_bstr_t	desc = e.Description();
	if (desc.length() > 0)
	{
		LPOLESTR lpolestr;
		IID iid = e.GUID();
		StringFromIID(iid, &lpolestr);
		_stprintf_s(msg, (sizeof(msg) / sizeof(wchar_t)), szFormat1, e.Error(), e.ErrorMessage(), 
					static_cast<LPTSTR>(desc), 
					static_cast<LPTSTR>(e.Source()), lpolestr);
		CoTaskMemFree(lpolestr);
	}
	else
	{
	    _stprintf_s(msg, (sizeof(msg) / sizeof(wchar_t)), szFormat2, e.Error(), e.ErrorMessage());
	}

	_ftprintf(stderr, L"\nERROR : %ws\n", msg);
	return;
}


void DisplayMessage(const wchar_t *pszFormat, ...)
{
	ASSERT(pszFormat);
	va_list args;
	va_start(args, pszFormat);

	_vsntprintf_s(msg, (sizeof(msg) / sizeof(wchar_t)) - 1, pszFormat, args);

	_ftprintf(stdout, L"%ws", msg);
	va_end(args);
	return;
}

const wchar_t* BuildMessage(const wchar_t *pszFormat, ...)
{
	ASSERT(pszFormat);
	va_list args;
	va_start(args, pszFormat);

	_vsntprintf_s(msg, (sizeof(msg) / sizeof(wchar_t)) - 1, pszFormat, args);

	va_end(args);
	return msg;
}

void TranslateErrorCode(DWORD dwErrorCode, _bstr_t &rMessage) 
{ 
    LPVOID lpMsgBuf;

	FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dwErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, 
		NULL );

	rMessage = (wchar_t*)lpMsgBuf;
    LocalFree(lpMsgBuf);
	return;
}


#ifdef _DEBUG
void _cdecl Trace(const wchar_t *pszFormat, ...)
{
	ASSERT(pszFormat);
	va_list args;
	va_start(args, pszFormat);

	_vsntprintf_s(msg, (sizeof(msg) / sizeof(wchar_t)) - 1, pszFormat, args);

	::OutputDebugString(msg);
	va_end(args);
	return;
}
#endif // _DEBUG


static char HexMsg[1024];
static const char *szFormatHex16   = "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X \n"; 

bool HexDump2File(FILE *pFile, const BYTE *pBuffer, unsigned nLength)
{
	ASSERT(pFile);
	ASSERT(pBuffer);

	if ((!pFile) || (!pBuffer))
		return false;

	fprintf(pFile, "\n");
	BYTE *pc = (BYTE*)pBuffer;
	unsigned nFullLines = nLength / 16;
	unsigned nRemaining = nLength % 16;

	for (unsigned lcv1 = 0; lcv1 < nFullLines; lcv1++)
	{
		sprintf_s(HexMsg, sizeof(HexMsg), szFormatHex16, pc[0],pc[1],pc[2],pc[3],pc[4],pc[5],pc[6],pc[7],pc[8],pc[9],pc[10],pc[11],pc[12],pc[13],pc[14],pc[15]);
		pc += 16;
		fprintf(pFile, "%s", HexMsg);
	}

	for (unsigned lcv2=0; lcv2 < nRemaining; lcv2++)
	{
		fprintf(pFile, "%02X ", pc[lcv2]);
	}
	fprintf(pFile, "\n");

	return true;
}


