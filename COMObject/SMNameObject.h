// SMNameObject.h : Declaration of the CSMNameObject

#pragma once
#include "resource.h"       // main symbols

#include <mutex>


#include "COMObject_i.h"
#include "atlcomcli.h"



#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


#define GUID_STRING_SIZE 39		// 39 - length of string representation of GUID + 1

// CSMNameObject

class ATL_NO_VTABLE CSMNameObject :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSMNameObject, &CLSID_SMNameObject>,
	public IDispatchImpl<ISMNameObject, &IID_ISMNameObject, &LIBID_COMObjectLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	DECLARE_CLASSFACTORY_SINGLETON(CSMNameObject)

	CSMNameObject()
		: m_bstrName(GUID_STRING_SIZE)
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SMNAMEOBJECT)


BEGIN_COM_MAP(CSMNameObject)
	COM_INTERFACE_ENTRY(ISMNameObject)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		HRESULT hr = GetRandomName(&m_bstrName.m_str);
		return hr;
	}

	void FinalRelease()
	{
	}

public:



	STDMETHOD(GetSharedMemoryName)(BSTR* name);
	STDMETHOD(GetNameSize)(ULONG* size);
	STDMETHOD(GetRandomName)(BSTR* name);
private:
	CComBSTR m_bstrName;
};

OBJECT_ENTRY_AUTO(__uuidof(SMNameObject), CSMNameObject)
