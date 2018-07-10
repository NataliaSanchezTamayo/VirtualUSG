#pragma once

#include <strsafe.h>
#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

EXTERN_C const GUID CLSID_VirtualCam;
EXTERN_C const GUID CLSID_PushSourceBitmapSender;

///////////////////////////////////////////////////
/*sections coming from pushsource
-------------------------------------------------*/
// UNITS = 10 ^ 7  
// UNITS / 30 = 30 fps;
// UNITS / 20 = 20 fps, etc
const REFERENCE_TIME FPS_30 = UNITS / 30;
const REFERENCE_TIME FPS_20 = UNITS / 20;
const REFERENCE_TIME FPS_10 = UNITS / 10;
const REFERENCE_TIME FPS_5 = UNITS / 5;
const REFERENCE_TIME FPS_4 = UNITS / 4;
const REFERENCE_TIME FPS_3 = UNITS / 3;
const REFERENCE_TIME FPS_2 = UNITS / 2;
const REFERENCE_TIME FPS_1 = UNITS / 1;

const REFERENCE_TIME rtDefaultFrameLength = FPS_10;
// Filter name strings
#define g_wszPushBitmapSender    L"PushSource Sender Filter"

/**********************************************
*
*  Class declarations
*
**********************************************/
class CPushPinBitmapSender : public CSourceStream
{
protected:

	int m_FramesWritten;				// To track where we are in the file
	BOOL m_bZeroMemory;                 // Do we need to clear the buffer?
	CRefTime m_rtSampleTime;	        // The time stamp for each sample

	BITMAPINFO *m_pBmi;                 // Pointer to the bitmap header
	DWORD       m_cbBitmapInfo;         // Size of the bitmap header

										// File opening variables 
	HANDLE m_hFile;                     // Handle returned from CreateFile
	BYTE * m_pFile;                     // Points to beginning of file buffer
	BYTE * m_pImage;                    // Points to pixel bits                                      

	int m_iFrameNumber;
	const REFERENCE_TIME m_rtFrameLength;

	CCritSec m_cSharedState;            // Protects our internal state
	CImageDisplay m_Display;            // Figures out our media type for us

public:

	CPushPinBitmapSender(HRESULT *phr, CSource *pFilter);
	~CPushPinBitmapSender();

	// Override the version that offers exactly one media type
	HRESULT GetMediaType(CMediaType *pMediaType);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
	HRESULT FillBuffer(IMediaSample *pSample);
	void UpDateHBitmap();

	// Quality control
	// Not implemented because we aren't going in real time.
	// If the file-writing filter slows the graph down, we just do nothing, which means
	// wait until we're unblocked. No frames are ever dropped.
	STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
	{
		return E_FAIL;
	}

};

class CPushSourceBitmapSender : public CSource
{

private:
	// Constructor is private because you have to use CreateInstance
	CPushSourceBitmapSender(IUnknown *pUnk, HRESULT *phr);
	~CPushSourceBitmapSender();

	CPushPinBitmapSender *m_pPin;

public:
	static CUnknown * WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);

};
///////////////////////////////////////////////////

class CVCamStream;
class CVCam : public CSource
{
public:
	//////////////////////////////////////////////////////////////////////////
	//  IUnknown
	//////////////////////////////////////////////////////////////////////////
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);

	IFilterGraph *GetGraph() { return m_pGraph; }

private:
	CVCam(LPUNKNOWN lpunk, HRESULT *phr);
};

class CVCamStream : public CSourceStream, public IAMStreamConfig, public IKsPropertySet
{

protected:
	HANDLE m_hFile;
	BYTE * m_pFile;
	BYTE * m_pImage;
	int m_iFrameNumber;
	const REFERENCE_TIME m_rtFrameLength;
	DWORD m_cbBitmapInfo;
	BITMAPINFO *m_pBmi;
	BOOL m_bZeroMemory;
	int m_FramesWritten;
public:

	//////////////////////////////////////////////////////////////////////////
	//  IUnknown
	//////////////////////////////////////////////////////////////////////////
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }                                                          \
		STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

	//////////////////////////////////////////////////////////////////////////
	//  IQualityControl
	//////////////////////////////////////////////////////////////////////////
	STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

	//////////////////////////////////////////////////////////////////////////
	//  IAMStreamConfig
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
	HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
	HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
	HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC);

	//////////////////////////////////////////////////////////////////////////
	//  IKsPropertySet
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData);
	HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData, DWORD *pcbReturned);
	HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);

	//////////////////////////////////////////////////////////////////////////
	//  CSourceStream
	//////////////////////////////////////////////////////////////////////////
	CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName);
	~CVCamStream();
	//NATI WAS HERE
	void UpDateHBitmap();

	HRESULT FillBuffer(IMediaSample *pms);
	HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT CheckMediaType(const CMediaType *pMediaType);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT SetMediaType(const CMediaType *pmt);
	HRESULT OnThreadCreate(void);

private:
	CVCam *m_pParent;
	REFERENCE_TIME m_rtLastTime;
	HBITMAP m_hLogoBmp;
	CCritSec m_cSharedState;
	IReferenceClock *m_pClock;

};


