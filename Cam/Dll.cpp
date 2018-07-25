//////////////////////////////////////////////////////////////////////////
//  This file contains routines to register / Unregister the 
//  Directshow filter 'Virtual Cam'
//  We do not use the inbuilt BaseClasses routines as we need to register as
//  a capture source
//////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "ole32")
#pragma comment(lib, "oleaut32")

#ifdef _DEBUG
#pragma comment(lib, "strmbasd")
#else
#pragma comment(lib, "strmbase")
#endif


#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <dllsetup.h>
#include "Filters.h"
#include <Mmsystem.h>
#include <Aviriff.h>

//this dll contains two filters only bitmap sender is being registered
//for some reason the image register in the original vcam filter is not displayed correctly in all applications
//push source filter is shown correctly as a filter in splitcam, manycam etc.
//vcam is shown correctly in teamviewer and as a filter, but in splitcam and vlc shows a blue screen not clear why.

#define CreateComObject(clsid, iid, var) CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, iid, (void **)&var);

STDAPI AMovieSetupRegisterServer(CLSID   clsServer, LPCWSTR szDescription, LPCWSTR szFileName, LPCWSTR szThreadingModel = L"Both", LPCWSTR szServerType = L"InprocServer32");
STDAPI AMovieSetupUnregisterServer(CLSID clsServer);



// {8E14549A-DB61-4309-AFA1-3578E927E933}
DEFINE_GUID(CLSID_VirtualCam,
	0x8e14549a, 0xdb61, 0x4309, 0xaf, 0xa1, 0x35, 0x78, 0xe9, 0x27, 0xe9, 0x33);

// {75BD8B7F-B1E4-4B0B-8132-CDF9FA141545}
DEFINE_GUID(CLSID_PushSourceBitmapSender,
	0x75bd8b7f, 0xb1e4, 0x4b0b, 0x81, 0x32, 0xcd, 0xf9, 0xfa, 0x14, 0x15, 0x45);

const AMOVIESETUP_MEDIATYPE AMSMediaTypesVCam =
{
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_NULL
};

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
	&MEDIATYPE_Video,       // Major type
	&MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN AMSPinVCam[] =
{
	{ L"Input",             // Pin's string name
	FALSE,                // Is it rendered
	FALSE,                // Is it an output
	FALSE,                // Allowed none
	FALSE,                // Allowed many
	&CLSID_NULL,          // Connects to filter
	L"Output",            // Connects to pin
	1,                    // Number of types
	&AMSMediaTypesVCam },       // Pin information

	{ L"Output",             // Pin string name
	FALSE,                 // Is it rendered
	TRUE,                  // Is it an output
	FALSE,                 // Can we have none
	FALSE,                 // Can we have many
	&CLSID_NULL,           // Connects to filter
	L"Input",                  // Connects to pin
	1,                     // Number of types
	&AMSMediaTypesVCam },     // Pin Media types

};

const AMOVIESETUP_PIN sudOutputPinBitmapSender[] =
{	
	{L"Input",             // Pin's string name
	FALSE,                // Is it rendered
	FALSE,                // Is it an output
	FALSE,                // Allowed none
	FALSE,                // Allowed many
	&CLSID_NULL,          // Connects to filter
	L"Output",            // Connects to pin
	1,                    // Number of types
	&sudOpPinTypes },       // Pin information

	{L"Output",      // Obsolete, not used.
	FALSE,          // Is this pin rendered?
	TRUE,           // Is it an output pin?
	FALSE,          // Can the filter create zero instances?
	FALSE,          // Does the filter create multiple instances?
	&CLSID_NULL,    // Obsolete.
	NULL,           // Obsolete.
	1,              // Number of media types.
	&sudOpPinTypes},  // Pointer to media types.
};

const AMOVIESETUP_FILTER sudPushSourceBitmapSender =
{
	&CLSID_PushSourceBitmapSender,// Filter CLSID
	g_wszPushBitmapSender,        // String name
	MERIT_DO_NOT_USE,       // Filter merit
	2,                      // Number pins
	sudOutputPinBitmapSender     // Pin details
};

const AMOVIESETUP_FILTER AMSFilterVCam =
{
	&CLSID_VirtualCam,  // Filter CLSID
	L"Virtual Cam",     // String name
	MERIT_DO_NOT_USE,      // Filter merit
	2,                     // Number pins
	AMSPinVCam             // Pin details
};

// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance.
// We provide a set of filters in this one DLL.

CFactoryTemplate g_Templates[2] =
{
	{
		g_wszPushBitmapSender,                // Name
		&CLSID_PushSourceBitmapSender,        // CLSID
		CPushSourceBitmapSender::CreateInstance,  // Method to create an instance of MyComponent
		NULL,                           // Initialization function
		&sudPushSourceBitmapSender            // Set-up information (for filters)
	},

	{
		L"Virtual Cam",
		&CLSID_VirtualCam,
		CVCam::CreateInstance,
		NULL,
		&AMSFilterVCam
	},

};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

//STDAPI RegisterFilters(BOOL bRegister)
//{
//	HRESULT hr = NOERROR;
//	WCHAR achFileName[MAX_PATH];
//	char achTemp[MAX_PATH];
//	ASSERT(g_hInst != 0);
//
//	if (0 == GetModuleFileNameA(g_hInst, achTemp, sizeof(achTemp)))
//		return AmHresultFromWin32(GetLastError());
//
//	MultiByteToWideChar(CP_ACP, 0L, achTemp, lstrlenA(achTemp) + 1,
//		achFileName, NUMELMS(achFileName));
//
//	hr = CoInitialize(0);
//	if (bRegister)
//	{
//		hr = AMovieSetupRegisterServer(CLSID_VirtualCam, L"Virtual Cam", achFileName, L"Both", L"InprocServer32");
//	}
//
//	if (SUCCEEDED(hr))
//	{
//		IFilterMapper2 *fm = 0;
//		hr = CreateComObject(CLSID_FilterMapper2, IID_IFilterMapper2, fm);
//		if (SUCCEEDED(hr))
//		{
//			if (bRegister)
//			{
//				IMoniker *pMoniker = 0;
//				REGFILTER2 rf2;
//				rf2.dwVersion = 1;
//				rf2.dwMerit = MERIT_DO_NOT_USE;
//				rf2.cPins = 2;
//				rf2.rgPins = AMSPinVCam;
//				hr = fm->RegisterFilter(CLSID_VirtualCam, L"Virtual Cam", &pMoniker, &CLSID_VideoInputDeviceCategory, NULL, &rf2);
//			}
//			else
//			{
//				hr = fm->UnregisterFilter(&CLSID_VideoInputDeviceCategory, 0, CLSID_VirtualCam);
//			}
//		}
//		// release interface
//		//
//		if (fm)
//			fm->Release();
//	}
//
//	if (SUCCEEDED(hr) && !bRegister)
//		hr = AMovieSetupUnregisterServer(CLSID_VirtualCam);
//
//	CoFreeUnusedLibraries();
//	CoUninitialize();
//	return hr;
//}

STDAPI RegisterFiltersBit(BOOL bRegister)
{
	HRESULT hr = NOERROR;
	WCHAR achFileName[MAX_PATH];
	char achTemp[MAX_PATH];
	ASSERT(g_hInst != 0);

	if (0 == GetModuleFileNameA(g_hInst, achTemp, sizeof(achTemp)))
		return AmHresultFromWin32(GetLastError());

	MultiByteToWideChar(CP_ACP, 0L, achTemp, lstrlenA(achTemp) + 1,
		achFileName, NUMELMS(achFileName));

	hr = CoInitialize(0);
	if (bRegister)
	{
		hr = AMovieSetupRegisterServer(CLSID_PushSourceBitmapSender, g_wszPushBitmapSender, achFileName, L"Both", L"InprocServer32");
	}

	if (SUCCEEDED(hr))
	{
		IFilterMapper2 *fm = 0;
		hr = CreateComObject(CLSID_FilterMapper2, IID_IFilterMapper2, fm);
		if (SUCCEEDED(hr))
		{
			if (bRegister)
			{
				IMoniker *pMoniker = 0;
				REGFILTER2 rf2;
				rf2.dwVersion = 1;
				rf2.dwMerit = MERIT_DO_NOT_USE;
				rf2.cPins = 2;
				rf2.rgPins = sudOutputPinBitmapSender;
				hr = fm->RegisterFilter(CLSID_PushSourceBitmapSender, g_wszPushBitmapSender, &pMoniker, &CLSID_VideoInputDeviceCategory, NULL, &rf2);
			}
			else
			{
				hr = fm->UnregisterFilter(&CLSID_VideoInputDeviceCategory, 0, CLSID_PushSourceBitmapSender);
			}
		}
		// release interface
		//
		if (fm)
			fm->Release();
	}

	if (SUCCEEDED(hr) && !bRegister)
		hr = AMovieSetupUnregisterServer(CLSID_PushSourceBitmapSender);

	CoFreeUnusedLibraries();
	CoUninitialize();
	return hr;
}

STDAPI DllRegisterServer()
{
	return RegisterFiltersBit(TRUE);
}

STDAPI DllUnregisterServer()
{
	return RegisterFiltersBit(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}
