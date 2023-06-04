//**************************************************************************
//  2008 Microsoft Corporation.  For illustration purposes only.
//**************************************************************************

#pragma once

#include "targetver.h"

//#define WIN32_LEAN_AND_MEAN			// Exclude rarely-used stuff from Windows headers
#define _WIN32_DCOM					// required for CoInitializeEx (could just use CoIntitialize)

#include <stdio.h>
#include <tchar.h>

#include <comdef.h>				// For the _bstr_t type and other COM+ extensions.
#include <Wbemidl.h>			// WMI header file
#include <devioctl.h>			// ensure the ddk header files are in your include path.
#include <ntddscsi.h>			//   e.g. .\WDK.H
#include <ntdddisk.h>			// Need the IDE_REGS struct for DeviceIoControl calls.
#include <vector>				// Minimal use of STL for managing multiple attached devices.
using namespace std;

//  Application global-scoped utility functions...

#ifdef _DEBUG
void _cdecl Trace(const wchar_t *pszFormat, ...);		
#define TRACE Trace
#define ASSERT assert
#else  
inline void _cdecl Trace(const wchar_t *, ...) {}
#define TRACE __noop
#define ASSERT __noop
#endif // _DEBUG

bool ValidOptions(int argc, _TCHAR *argv[]);
void DisplayUsage(const wchar_t *progname);
void DisplayErrorMessage(const HRESULT& hr);
void DisplayErrorMessage(const wchar_t *szError);
void DisplayErrorMessage(_com_error &e);   
void DisplayMessage(const wchar_t *pszFormat, ...);	
const wchar_t* BuildMessage(const wchar_t *pszFormat, ...);
void TranslateErrorCode(DWORD dwErrorCode, _bstr_t &rMessage);

inline unsigned __int32 n32ByteSwap(unsigned __int32 dwOriginal)
{
  return (((dwOriginal << 24) & 0xFF000000) |
		  ((dwOriginal <<  8) & 0x00FF0000) |
          ((dwOriginal >>  8) & 0x0000FF00) |
          ((dwOriginal >> 24) & 0x000000FF));  
}

inline unsigned __int16 n16ByteSwap(unsigned __int16 nOriginal)
{
  return (((nOriginal << 8) & 0xFF00) |
          ((nOriginal >> 8) & 0x00FF));
}

bool HexDump2File(FILE *pfile, const BYTE *pBuffer, unsigned nLength);

// TODO: reference additional headers your program requires here
