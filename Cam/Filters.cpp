#pragma warning(disable:4244)
#pragma warning(disable:4711)

#include <streams.h>
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include "Filters.h"
#include <strmif.h>
#include <Dshow.h>

#define BITMAP_NAME TEXT("Bitmap1.bmp")
//////////////////////////////////////////////////////////////////////////
//  CVCam is the source filter which masquerades as a capture device
//////////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CVCam::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);
	CUnknown *punk = new CVCam(lpunk, phr);
	return punk;
}

CVCam::CVCam(LPUNKNOWN lpunk, HRESULT *phr) :
	CSource(NAME("Virtual Cam"), lpunk, CLSID_VirtualCam)
{
	ASSERT(phr);
	CAutoLock cAutoLock(&m_cStateLock);
	// Create the one and only output pin
	m_paStreams = (CSourceStream **) new CVCamStream*[1];
	m_paStreams[0] = new CVCamStream(phr, this, L"Virtual Cam");
}

HRESULT CVCam::QueryInterface(REFIID riid, void **ppv)
{
	//Forward request for IAMStreamConfig & IKsPropertySet to the pin
	if (riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
		return m_paStreams[0]->QueryInterface(riid, ppv);
	else
		return CSource::QueryInterface(riid, ppv);
}


//////////////////////////////////////////////////////////////////////////
// CVCamStream is the one and only output pin of CVCam which handles 
// all the stuff.
//////////////////////////////////////////////////////////////////////////
CVCamStream::CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName) :
	CSourceStream(NAME("Virtual Cam"), phr, pParent, pPinName), m_pParent(pParent),
	m_FramesWritten(0),
	m_bZeroMemory(0),
	m_pBmi(0),
	m_cbBitmapInfo(0),
	m_hFile(INVALID_HANDLE_VALUE),
	m_pFile(NULL),
	m_pImage(NULL),
	m_iFrameNumber(0),
	m_rtFrameLength(FPS_30) // Display 30 bitmap frames per second
{
	
	//************************* Changed by Nati 07/10/2018*******///////////////
	TCHAR szCurrentDir[MAX_PATH], szFileCurrent[MAX_PATH], szFileMedia[MAX_PATH];
	TCHAR newDir[MAX_PATH] = TEXT("C:\\Users\\natal\\Desktop");
	// First look for the bitmap in the current directory
	GetCurrentDirectory(MAX_PATH - 1, szCurrentDir);
	(void)StringCchPrintf(szFileCurrent, NUMELMS(szFileCurrent), TEXT("%s\\%s\0"), newDir, BITMAP_NAME);
	m_hFile = CreateFile(szFileCurrent, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		// File was not in the application's current directory,
		// so look in the DirectX SDK media path instead.
		//(void)StringCchCopy(szFileMedia, NUMELMS(szFileMedia), DXUtil_GetDXSDKMediaPath());
		(void)StringCchCat(szFileMedia, NUMELMS(szFileMedia), BITMAP_NAME);

		m_hFile = CreateFile(szFileMedia, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);

		if (m_hFile == INVALID_HANDLE_VALUE)
		{
			TCHAR szMsg[MAX_PATH + MAX_PATH + 100];

			(void)StringCchPrintf(szMsg, NUMELMS(szMsg), TEXT("Could not open bitmap source file in the application directory:\r\n\r\n\t[%s]\n\n")
				TEXT("or in the DirectX SDK Media folder:\r\n\r\n\t[%s]\n\n")
				TEXT("Please copy this file either to the application's folder\r\n")
				TEXT("or to the DirectX SDK Media folder, then recreate this filter.\r\n")
				TEXT("Otherwise, you will not be able to render the output pin.\0"),
				szFileCurrent, szFileMedia);

			OutputDebugString(szMsg);
			MessageBox(NULL, szMsg, TEXT("PushSource filter error"), MB_ICONERROR | MB_OK);
			*phr = HRESULT_FROM_WIN32(GetLastError());
			return;
		}
	}
	DWORD dwFileSize = GetFileSize(m_hFile, NULL);
	if (dwFileSize == INVALID_FILE_SIZE)
	{
		DbgLog((LOG_TRACE, 1, TEXT("Invalid file size")));
		*phr = HRESULT_FROM_WIN32(GetLastError());
		return;
	}

	m_pFile = new BYTE[dwFileSize];
	if (!m_pFile)
	{
		OutputDebugString(TEXT("Could not allocate m_pImage\n"));
		*phr = E_OUTOFMEMORY;
		return;
	}

	DWORD nBytesRead = 0;
	if (!ReadFile(m_hFile, m_pFile, dwFileSize, &nBytesRead, NULL))
	{
		*phr = HRESULT_FROM_WIN32(GetLastError());
		OutputDebugString(TEXT("ReadFile failed\n"));
		return;
	}

	// WARNING - This code does not verify that the file is a valid bitmap file.
	// In your own filter, you would check this or else generate the bitmaps 
	// yourself in memory.

	int cbFileHeader = sizeof(BITMAPFILEHEADER);

	// Store the size of the BITMAPINFO 
	BITMAPFILEHEADER *pBm = (BITMAPFILEHEADER*)m_pFile;
	m_cbBitmapInfo = pBm->bfOffBits - cbFileHeader;

	// Store a pointer to the BITMAPINFO
	m_pBmi = (BITMAPINFO*)(m_pFile + cbFileHeader);

	// Store a pointer to the starting address of the pixel bits
	m_pImage = m_pFile + cbFileHeader + m_cbBitmapInfo;

	// Close and invalidate the file handle, since we have copied its bitmap data
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;

	////////////
	// Set the default media type as 320x240x24@15
	GetMediaType(4, &m_mt);
}

void CVCamStream::UpDateHBitmap()
{

	TCHAR szCurrentDir[MAX_PATH], szFileCurrent[MAX_PATH], szFileMedia[MAX_PATH];
	TCHAR newDir[MAX_PATH] = TEXT("C:\\Users\\natal\\Desktop");

	//(void)StringCchPrintf(szFileCurrent, NUMELMS(szFileCurrent), TEXT("%s\\%s\0"), szCurrentDir, BITMAP_NAME);

	// ************************** CHANGED BY MARU 07/09/18 at 5:39PM ***************************** //
	(void)StringCchPrintf(szFileCurrent, NUMELMS(szFileCurrent), TEXT("%s\\%s\0"), newDir, BITMAP_NAME);

	m_hFile = CreateFile(szFileCurrent, GENERIC_READ, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);

	if (m_hFile == INVALID_HANDLE_VALUE)
	{
		// File was not in the application's current directory,
		// so look in the DirectX SDK media path instead.
		//(void)StringCchCopy(szFileMedia, NUMELMS(szFileMedia), DXUtil_GetDXSDKMediaPath());
		(void)StringCchCat(szFileMedia, NUMELMS(szFileMedia), BITMAP_NAME);

		m_hFile = CreateFile(szFileMedia, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);


	}

	DWORD dwFileSize = GetFileSize(m_hFile, NULL);
	if (dwFileSize == INVALID_FILE_SIZE)
	{
		DbgLog((LOG_TRACE, 1, TEXT("Invalid file size")));
		return;
	}


	DWORD nBytesRead = 0;
	if (!ReadFile(m_hFile, m_pFile, dwFileSize, &nBytesRead, NULL))
	{
		//*phr = HRESULT_FROM_WIN32(GetLastError());
		OutputDebugString(TEXT("ReadFile failed\n"));
		return;
	}

	// WARNING - This code does not verify that the file is a valid bitmap file.
	// In your own filter, you would check this or else generate the bitmaps 
	// yourself in memory.

	int cbFileHeader = sizeof(BITMAPFILEHEADER);

	// Store the size of the BITMAPINFO 
	BITMAPFILEHEADER *pBm = (BITMAPFILEHEADER*)m_pFile;
	m_cbBitmapInfo = pBm->bfOffBits - cbFileHeader;

	// Store a pointer to the BITMAPINFO
	m_pBmi = (BITMAPINFO*)(m_pFile + cbFileHeader);

	// Store a pointer to the starting address of the pixel bits
	m_pImage = m_pFile + cbFileHeader + m_cbBitmapInfo;

	// Close and invalidate the file handle, since we have copied its bitmap data
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
}


CVCamStream::~CVCamStream()
{
	DbgLog((LOG_TRACE, 3, TEXT("Frames written %d"), m_iFrameNumber));

	if (m_pFile)
	{
		delete[] m_pFile;
	}

	// The constructor might quit early on error and not close the file...
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
	}
}

HRESULT CVCamStream::QueryInterface(REFIID riid, void **ppv)
{
	// Standard OLE stuff
	if (riid == _uuidof(IAMStreamConfig))
		*ppv = (IAMStreamConfig*)this;
	else if (riid == _uuidof(IKsPropertySet))
		*ppv = (IKsPropertySet*)this;
	else
		return CSourceStream::QueryInterface(riid, ppv);

	AddRef();
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//  This is the routine where we create the data being output by the Virtual
//  Camera device.
//////////////////////////////////////////////////////////////////////////

HRESULT CVCamStream::FillBuffer(IMediaSample *pms)
{
	REFERENCE_TIME rtNow;

	REFERENCE_TIME avgFrameTime = ((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame;

	rtNow = m_rtLastTime;
	m_rtLastTime += avgFrameTime;
	pms->SetTime(&rtNow, &m_rtLastTime);
	pms->SetSyncPoint(TRUE);

	BYTE *pData;
	//long lDataLen;
	long cbData;

	CheckPointer(pms, E_POINTER);

	// If the bitmap file was not loaded, just fail here.
	if (!m_pImage)
		return E_FAIL;

	pms->GetPointer(&pData);
	//lDataLen = pms->GetSize();
	cbData = pms ->GetSize();
	// Check that we're still using video
	ASSERT(m_mt.formattype == FORMAT_VideoInfo);
	VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)m_mt.pbFormat;
	// If we want to change the contents of our source buffer (m_pImage)
	// at some interval or based on some condition, this is where to do it.
	// Remember that the new data has the same format that we specified in GetMediaType.
	// For example: 
	//if (m_iFrameNumber > SomeValue)
	//	LoadNewBitsIntoBuffer(m_pImage);

	// Copy the DIB bits over into our filter's output buffer.
	// Since sample size may be larger than the image size, bound the copy size.
	UpDateHBitmap();
	// Copy the bitmap data into the provided BYTE buffer
	memcpy(pData, m_pImage, min(pVih->bmiHeader.biSizeImage, (DWORD)cbData));
	// Set the timestamps that will govern playback frame rate.
	// If this file is getting written out as an AVI,
	// then you'll also need to configure the AVI Mux filter to 
	// set the Average Time Per Frame for the AVI Header.
	// The current time is the sample's start
	REFERENCE_TIME rtStart = m_iFrameNumber * m_rtFrameLength;
	REFERENCE_TIME rtStop = rtStart + m_rtFrameLength;

	pms->SetTime(&rtStart, &rtStop);
	m_iFrameNumber++;

	// Set TRUE on every sample for uncompressed frames
	pms->SetSyncPoint(TRUE);
	//for (int i = 0; i < lDataLen; ++i)
		//pData[i] = rand();

	return NOERROR;
} // FillBuffer


  //
  // Notify
  // Ignore quality management messages sent from the downstream filter
STDMETHODIMP CVCamStream::Notify(IBaseFilter * pSender, Quality q)
{
	return E_NOTIMPL;
} // Notify

  //////////////////////////////////////////////////////////////////////////
  // This is called when the output format has been negotiated
  //////////////////////////////////////////////////////////////////////////
HRESULT CVCamStream::SetMediaType(const CMediaType *pmt)
{
	DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
	HRESULT hr = CSourceStream::SetMediaType(pmt);
	return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CVCamStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	if (iPosition < 0) return E_INVALIDARG;
	if (iPosition > 8) return VFW_S_NO_MORE_ITEMS;

	if (iPosition == 0)
	{
		*pmt = m_mt;
		return S_OK;
	}

	DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
	ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

	//NATI---7/10
	// Allocate enough room for the VIDEOINFOHEADER and the color tables
	//VIDEOINFOHEADER *pviNAT = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(SIZE_PREHEADER + m_cbBitmapInfo);
	// Copy the header info
	//memcpy(&(pviNAT->bmiHeader), m_pBmi, m_cbBitmapInfo);

	// Copy the header info
	//memcpy(&(pvi->bmiHeader), m_pBmi, m_cbBitmapInfo);

	//pvi->bmiHeader.biCompression = BI_RGB;
	pvi->bmiHeader.biBitCount = 32;// previous 24
	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	//pvi->bmiHeader.biWidth = 80 * iPosition;
	//pvi->bmiHeader.biHeight =60 * iPosition;
	pvi->bmiHeader.biWidth = 512;
	pvi->bmiHeader.biHeight = 512;
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
	pvi->bmiHeader.biClrImportant = 0;

	//pvi->AvgTimePerFrame = 1000000;
	pvi->AvgTimePerFrame = m_rtFrameLength;
	SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
	SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

	pmt->SetType(&MEDIATYPE_Video);
	pmt->SetFormatType(&FORMAT_VideoInfo);
	pmt->SetTemporalCompression(FALSE);

	// Work out the GUID for the subtype from the header info.
	const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
	pmt->SetSubtype(&SubTypeGUID);
	pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

	return NOERROR;

} // GetMediaType

  // This method is called to see if a given output format is supported
HRESULT CVCamStream::CheckMediaType(const CMediaType *pMediaType)
{
	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pMediaType->Format());
	if (*pMediaType != m_mt)
		return E_INVALIDARG;
	return S_OK;
} // CheckMediaType

  // This method is called after the pins are connected to allocate buffers to stream data
HRESULT CVCamStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock());
	HRESULT hr = NOERROR;

	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(pProperties, E_POINTER);
	// If the bitmap file was not loaded, just fail here.
	if (!m_pImage)
		return E_FAIL;

	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)m_mt.Format();
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties, &Actual);

	if (FAILED(hr)) return hr;
	if (Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;

	return NOERROR;
} // DecideBufferSize

  // Called when graph is run
HRESULT CVCamStream::OnThreadCreate()
{
	m_rtLastTime = 0;
	return NOERROR;
} // OnThreadCreate


  //////////////////////////////////////////////////////////////////////////
  //  IAMStreamConfig
  //////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CVCamStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
	DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.pbFormat);
	m_mt = *pmt;
	IPin* pin;
	ConnectedTo(&pin);
	if (pin)
	{
		IFilterGraph *pGraph = m_pParent->GetGraph();
		pGraph->Reconnect(this);
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
	*ppmt = CreateMediaType(&m_mt);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
	*piCount = 8;
	*piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
	*pmt = CreateMediaType(&m_mt);
	DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);

	if (iIndex == 0) iIndex = 4;

	pvi->bmiHeader.biCompression = BI_RGB;
	pvi->bmiHeader.biBitCount = 24;
	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biWidth = 80 * iIndex;
	pvi->bmiHeader.biHeight = 60 * iIndex;
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
	pvi->bmiHeader.biClrImportant = 0;

	SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
	SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

	(*pmt)->majortype = MEDIATYPE_Video;
	(*pmt)->subtype = MEDIASUBTYPE_RGB555;
	(*pmt)->formattype = FORMAT_VideoInfo;
	(*pmt)->bTemporalCompression = FALSE;
	(*pmt)->bFixedSizeSamples = FALSE;
	(*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
	(*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);

	DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);

	pvscc->guid = FORMAT_VideoInfo;
	pvscc->VideoStandard = AnalogVideo_None;
	pvscc->InputSize.cx = 640;
	pvscc->InputSize.cy = 480;
	pvscc->MinCroppingSize.cx = 80;
	pvscc->MinCroppingSize.cy = 60;
	pvscc->MaxCroppingSize.cx = 640;
	pvscc->MaxCroppingSize.cy = 480;
	pvscc->CropGranularityX = 80;
	pvscc->CropGranularityY = 60;
	pvscc->CropAlignX = 0;
	pvscc->CropAlignY = 0;

	pvscc->MinOutputSize.cx = 80;
	pvscc->MinOutputSize.cy = 60;
	pvscc->MaxOutputSize.cx = 640;
	pvscc->MaxOutputSize.cy = 480;
	pvscc->OutputGranularityX = 0;
	pvscc->OutputGranularityY = 0;
	pvscc->StretchTapsX = 0;
	pvscc->StretchTapsY = 0;
	pvscc->ShrinkTapsX = 0;
	pvscc->ShrinkTapsY = 0;
	pvscc->MinFrameInterval = 200000;   //50 fps
	pvscc->MaxFrameInterval = 50000000; // 0.2 fps
	pvscc->MinBitsPerSecond = (80 * 60 * 3 * 8) / 5;
	pvscc->MaxBitsPerSecond = 640 * 480 * 3 * 8 * 50;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////


HRESULT CVCamStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData,
	DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
	return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CVCamStream::Get(
	REFGUID guidPropSet,   // Which property set.
	DWORD dwPropID,        // Which property in that set.
	void *pInstanceData,   // Instance data (ignore).
	DWORD cbInstanceData,  // Size of the instance data (ignore).
	void *pPropData,       // Buffer to receive the property data.
	DWORD cbPropData,      // Size of the buffer.
	DWORD *pcbReturned     // Return the size of the property.
)
{
	if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
	if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;

	if (pcbReturned) *pcbReturned = sizeof(GUID);
	if (pPropData == NULL)          return S_OK; // Caller just wants to know the size. 
	if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;// The buffer is too small.

	*(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
	return S_OK;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT CVCamStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
	if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
	// We support getting this property, but not setting it.
	if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	return S_OK;
}
