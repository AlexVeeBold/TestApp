// COMObject.cpp : Implementation of WinMain


#include "stdafx.h"
#include "resource.h"
#include "COMObject_i.h"


using namespace ATL;


class CCOMObjectModule : public ATL::CAtlExeModuleT< CCOMObjectModule >
{
public :
	DECLARE_LIBID(LIBID_COMObjectLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_COMOBJECT, "{2F470D0B-276C-4E7F-A91A-6DAEA86F5277}")
	};

CCOMObjectModule _AtlModule;



//
extern "C" int WINAPI _tWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, 
								LPTSTR /*lpCmdLine*/, int nShowCmd)
{
	return _AtlModule.WinMain(nShowCmd);
}

