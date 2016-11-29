#pragma once

#include <string>

#include "COMObject/COMObject_i.h"


class CSMNameServer
{
private:
	ISMNameObject* m_pISMNameObject;
	BSTR allocCOMString();
	std::string stringFromCOMString(BSTR bstrString);

public:
	CSMNameServer(void);
	~CSMNameServer(void);

	bool isCreated(void);

	std::string getSharedMemoryName(void);
	std::string getRandomName(void);
};

