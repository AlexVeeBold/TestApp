// SMClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <boost/filesystem.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

namespace ipc {
	using namespace boost::interprocess;
};
namespace fs {
	using namespace boost::filesystem;
};
namespace opt {
	using namespace boost::program_options;
};


#include "SMCommon/SMTransfer.h"


class CSMClient
{
private:
	CSMTransfer m_transferMain;
	void doTransfer(std::string strFilePath);
	void dispatchNewTransfer(void);

public:
	CSMClient()
	{
	}
	~CSMClient()
	{
	}
	
	void launch(std::vector<std::string>& files);
};


void CSMClient::doTransfer(std::string strFilePath)
{
	CSMTransfer transfer;
	std::string fileName = fs::path(strFilePath.c_str()).filename().string();

	// create client & open server message queues
	transfer.createClientToServerNode(strFilePath);
	std::cout << "Client Name: " << transfer.getClientQueueName() << std::endl;

	// send request to server
	std::cout << "Requesting file transfer..." << std::endl;
	transfer.msgMakeAsk(fileName);
	transfer.sendAndReceive();

	CSMTransfer::MsgType type = transfer.getMessageType();
	if(type == CSMTransfer::MsgType::allow)
	{
		// open message queue passed by server
		transfer.updateClientToServerNode();
		// create/open memory/file mapping objects
		transfer.createClientMemoryObjects();

		bool bContinue;
		std::stringstream ssProgress;
		do {
			// transfer block
			bContinue = transfer.regionSend();
			// display progress
			ssProgress.str("");
			ssProgress << transfer.getFileName() << " (" << transfer.getFileOffset() << " / " << transfer.getFileSize() << ")";
			if(!bContinue)
			{
				ssProgress << " - done";
			}
			ssProgress << std::endl;
			std::cout << ssProgress.str();
		} while(bContinue);
	}
	else
	{
		std::cout << "Unable to send " << transfer.getFileName() << std::endl;
	}
}

void CSMClient::launch(std::vector<std::string>& files)
{
	if(m_transferMain.isNameServerAvailable() == false)
	{
		std::cout << "Couldn't connect to COM NameServer" << std::endl;
		return;
	}

	try {
		std::cout << "Detecting server..." << std::endl;
		m_transferMain.createRemoteServerNode();
		std::cout << "Server Name: " << m_transferMain.getServerQueueName() << std::endl << std::endl;

		std::vector<std::string>::iterator it = files.begin();
		while(it != files.end())
		{
			doTransfer(*it);
			it++;
		}
	}
	catch(ipc::interprocess_exception& e) {
		std::cout << "interprocess_exception : " << e.what() << std::endl;
	}
	catch(fs::filesystem_error& e) {
		std::cout << "filesystem_error : " << e.what() << std::endl;
	}
}


int _tmain(int argc, _TCHAR* argv[])
{
	::SetConsoleOutputCP(::GetACP());

	// setup options
	opt::options_description desc("Options");
	desc.add_options()
		("help,?", "Display this help message")
		("file,F", opt::value< std::vector<std::string> >(), "File(s) to transfer:\n-F one_file.bin -F \"and another one.dat\"")
	;

	// load options from command line
	opt::variables_map vmap;
	auto options = opt::basic_command_line_parser<_TCHAR>(argc, argv).options(desc).allow_unregistered().run();
	opt::store(options, vmap);
	opt::notify(vmap);

	// process options
	if((vmap.size() == 0) || vmap.count("help"))
	{
		std::cout << desc << std::endl;
		return 1;
	}
	if(vmap.count("file"))
	{
		std::vector<std::string> files = vmap["file"].as< std::vector<std::string> >();

		CSMClient smClient;
		smClient.launch(files);
	}

	return 0;
}

