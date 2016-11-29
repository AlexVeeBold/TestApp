// SMNameObject.cpp : Implementation of CSMNameObject

#include "stdafx.h"
#include "SMNameObject.h"


// CSMNameObject


STDMETHODIMP CSMNameObject::GetSharedMemoryName(BSTR* name)
{
	return m_bstrName.CopyTo(name);
}


STDMETHODIMP CSMNameObject::GetNameSize(ULONG* size)
{
	*size = GUID_STRING_SIZE;
	return S_OK;
}


STDMETHODIMP CSMNameObject::GetRandomName(BSTR* name)
{
	GUID guid;
	HRESULT hr = ::CoCreateGuid(&guid);
	CComBSTR bstrGuid(guid);
	return bstrGuid.CopyTo(name);
}
