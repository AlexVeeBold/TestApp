#include "stdafx.h"
#include "SMNameServer.h"

#include "COMObject/COMObject_i.c"


CSMNameServer::CSMNameServer(void)
{
	::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	HRESULT hr = ::CoCreateInstance(CLSID_SMNameObject, nullptr, CLSCTX_LOCAL_SERVER, IID_ISMNameObject, (void**)&m_pISMNameObject);
}

CSMNameServer::~CSMNameServer(void)
{
	if(m_pISMNameObject != nullptr)
	{
		m_pISMNameObject->Release();
		m_pISMNameObject = nullptr;
	}
	::CoUninitialize();
}


bool CSMNameServer::isCreated(void)
{
	return (m_pISMNameObject != nullptr);
}


BSTR CSMNameServer::allocCOMString(void)
{
	ULONG size;
	m_pISMNameObject->GetNameSize(&size);
	std::wstring wstr(size, L'\0');
	BSTR bstrName = ::SysAllocString(wstr.c_str());
	return bstrName;
}

std::string CSMNameServer::stringFromCOMString(BSTR bstrString)
{
	std::string smName;
	std::wstring wideName(bstrString, ::SysStringLen(bstrString));
	smName.assign(wideName.begin(), wideName.end());
	::SysFreeString(bstrString);
	return smName;
}


std::string CSMNameServer::getSharedMemoryName(void)
{
	BSTR bstrName = allocCOMString();
	m_pISMNameObject->GetSharedMemoryName(&bstrName);
	return stringFromCOMString(bstrName);
}

std::string CSMNameServer::getRandomName(void)
{
	BSTR bstrName = allocCOMString();
	m_pISMNameObject->GetRandomName(&bstrName);
	return stringFromCOMString(bstrName);
}

