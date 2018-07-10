CMyFilter: public CBaseFilter, public IMyCustomInterface
{
public:
	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID iid, void **ppv);
};
		   STDMETHODIMP CMyFilter::NonDelegatingQueryInterface(REFIID iid, void **ppv)
		   {
			   if (riid == IID_IMyCustomInterface) {
				   return GetInterface(static_cast<IMyCustomInterface*>(this), ppv);
			   }
			   return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
		   }

		   For more information, see How to Implement IUnknown.

			   Object Creation

			   If you plan to package your filter in a DLL and make it available to other clients, you must support CoCreateInstance and other related COM functions.The base class library implements most of this; you just need to provide some information about your filter.This section gives a brief overview of what to do.For details, see How to Create a DLL.

			   First, write a static class method that returns a new instance of your filter.You can name this method anything you like, but the signature must match the one shown in the following example :

		   CUnknown * WINAPI CRleFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr)
		   {
			   CRleFilter *pFilter = new CRleFilter();
			   if (pFilter == NULL)
			   {
				   *pHr = E_OUTOFMEMORY;
			   }
			   return pFilter;
		   }

		   Next, declare a global array of CFactoryTemplate class instances, named g_Templates.Each CFactoryTemplate class contains registry information for one filter.Several filters can reside in a single DLL; simply include additional CFactoryTemplate entries.You can also declare other COM objects, such as property pages.

			   static WCHAR g_wszName[] = L"My RLE Encoder";
		   CFactoryTemplate g_Templates[] =
		   {
			   {
				   g_wszName,
				   &CLSID_RLEFilter,
				   CRleFilter::CreateInstance,
				   NULL,
				   NULL
			   }
		   };

		   Define a global integer named g_cTemplates whose value equals the length of the g_Templates array:

		   int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

		   Finally, implement the DLL registration functions.The following example shows the minimal implementation for these functions :

		   STDAPI DllRegisterServer()
		   {
			   return AMovieDllRegisterServer2(TRUE);
		   }
		   STDAPI DllUnregisterServer()
		   {
			   return AMovieDllRegisterServer2(FALSE);
		   }

		   Filter Registry Entries

			   The previous examples show how to register a filter's CLSID for COM. For many filters, this is sufficient. The client is then expected to create the filter using CoCreateInstance and add it to the filter graph by calling IFilterGraph::AddFilter. In some cases, however, you might want to provide additional information about the filter in the registry. This information does the following:

			   Enables clients to discover the filter using the Filter Mapper or the System Device Enumerator.
			   Enables the Filter Graph Manager to discover the filter during automatic graph building.

			   The following example registers the RLE encoder filter in the video compressor category.For details, see How to Register DirectShow Filters.Be sure to read the section Guidelines for Registering Filters, which describes the recommended practices for filter registration.

			   // Declare media type information.
			   FOURCCMap fccMap = FCC('MRLE');
		   REGPINTYPES sudInputTypes = { &MEDIATYPE_Video, &GUID_NULL };
		   REGPINTYPES sudOutputTypes = { &MEDIATYPE_Video, (GUID*)&fccMap };

		   // Declare pin information.
		   REGFILTERPINS sudPinReg[] = {
			   // Input pin.
			   { 0, FALSE, // Rendered?
			   FALSE, // Output?
			   FALSE, // Zero?
			   FALSE, // Many?
			   0, 0,
			   1, &sudInputTypes  // Media types.
			   },
			   // Output pin.
			   { 0, FALSE, // Rendered?
			   TRUE, // Output?
			   FALSE, // Zero?
			   FALSE, // Many?
			   0, 0,
			   1, &sudOutputTypes      // Media types.
			   }
		   };

		   // Declare filter information.
		   REGFILTER2 rf2FilterReg = {
			   1,                // Version number.
			   MERIT_DO_NOT_USE, // Merit.
			   2,                // Number of pins.
			   sudPinReg         // Pointer to pin information.
		   };

		   STDAPI DllRegisterServer(void)
		   {
			   HRESULT hr = AMovieDllRegisterServer2(TRUE);
			   if (FAILED(hr))
			   {
				   return hr;
			   }
			   IFilterMapper2 *pFM2 = NULL;
			   hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
				   IID_IFilterMapper2, (void **)&pFM2);
			   if (SUCCEEDED(hr))
			   {
				   hr = pFM2->RegisterFilter(
					   CLSID_RLEFilter,                // Filter CLSID. 
					   g_wszName,                       // Filter name.
					   NULL,                            // Device moniker. 
					   &CLSID_VideoCompressorCategory,  // Video compressor category.
					   g_wszName,                       // Instance data.
					   &rf2FilterReg                    // Filter information.
				   );
				   pFM2->Release();
			   }
			   return hr;
		   }

		   STDAPI DllUnregisterServer()
		   {
			   HRESULT hr = AMovieDllRegisterServer2(FALSE);
			   if (FAILED(hr))
			   {
				   return hr;
			   }
			   IFilterMapper2 *pFM2 = NULL;
			   hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
				   IID_IFilterMapper2, (void **)&pFM2);
			   if (SUCCEEDED(hr))
			   {
				   hr = pFM2->UnregisterFilter(&CLSID_VideoCompressorCategory,
					   g_wszName, CLSID_RLEFilter);
				   pFM2->Release();
			   }
			   return hr;
		   }

		   Also, filters do not have to be packaged inside DLLs.In some cases, you might write a specialized filter that is designed only for a specific application.In that case, you can compile the filter class directly in your application, and create it with the new operator, as shown in the following example :

#include "MyFilter.h"  // Header file that declares the filter class.
		   // Compile and link MyFilter.cpp.
		   int main()
		   {
			   IBaseFilter *pFilter = 0;
			   {
				   // Scope to hide pF.
				   CMyFilter* pF = new MyFilter();
				   if (!pF)
				   {
					   printf("Could not create MyFilter.\n");
					   return 1;
				   }
				   pF->QueryInterface(IID_IBaseFilter,
					   reinterpret_cast<void**>(&pFilter));
			   }

			   /* Now use pFilter as normal. */

			   pFilter->Release();  // Deletes the filter.
			   return 0;
		   }
