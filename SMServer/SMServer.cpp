// SMServer.cpp : Defines the entry point for the console application.
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
#include <boost/thread/thread.hpp>

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


class CSMServer
{
private:
	CSMTransfer m_transferMain;
	const char* m_errorMsg;
	std::string m_recvdDirName;
	void transferThread(CSMTransfer transfer);
	void run(void);
	void dispatchNewTransfer(void);
	void dispatchCurrentTransfer(std::uint32_t m_serverTransferId);

public:
	CSMServer()
		: m_recvdDirName("./recvd/")
	{
	}
	~CSMServer()
	{
	}

	void launch(void);
};


void CSMServer::transferThread(CSMTransfer foreign)
{
	// 1gb / 8 clients = 128 mb per client
	// 128 mb / 4 files = 32 mb per file
	std::size_t maxMemory = 32 * 1048576; // 32mb
	std::size_t pageSize = ipc::mapped_region::get_page_size();
	std::size_t pageCount = maxMemory / pageSize;
	std::size_t regionSize = pageSize * pageCount;
	//std::size_t regionSize = pageSize;

	try {
		// create thread-local server message queue
		CSMTransfer local;
		local.createServerToClientTransferNode(foreign);

		std::string filePath;
		std::string fileName = local.getFileName();
		filePath.append(m_recvdDirName);
		filePath.append(fileName);

		// create shared memory & file mapping objects
		local.createServerMemoryObjects(filePath);
		local.msgMakeAllow(regionSize);
		local.send();

		bool bContinue;
		std::stringstream ssProgress;
		do {
			// receive block
			bContinue = local.regionReceive();
			// display progress
			ssProgress.str("");
			ssProgress << local.getFileName() << " : " << local.getFileOffset() << "/" << local.getFileSize();
			if(!bContinue)
			{
				ssProgress << " - done";
			}
			ssProgress << std::endl;
			std::cout << ssProgress.str();
		} while(bContinue);
	}
	catch(ipc::interprocess_exception& e) {
		std::cout << "interprocess_exception : " << e.what() << std::endl;
	}
	catch(fs::filesystem_error& e) {
		std::cout << "filesystem_error : " << e.what() << std::endl;
	}
	//catch(boost::archive::archive_exception& e) {
	//	std::cout << "<T>: archive_exception" << std::endl << e.what() << std::endl;
	//}
}

void CSMServer::dispatchNewTransfer(void)
{
	fs::space_info si;
	si = fs::space(fs::path("."));
	std::uintmax_t fileSize = m_transferMain.getFileSize();

	if(fileSize >= si.available)
	{
		std::cout << "Not enough space for file " << m_transferMain.getFileName() << std::endl;
		std::cout << "Available: " << si.available << ", file size: " << fileSize << std::endl;
		// denial
		std::cout << "Denying to " << m_transferMain.getClientQueueName() << std::endl;
		CSMTransfer reply;
		reply.msgMakeDeny();
		reply.createServerToClientNotifyNode(m_transferMain);
		reply.send();
	}
	else
	{
		// allowance
		boost::thread workerThread(&CSMServer::transferThread, this, m_transferMain);
	}	
}


void CSMServer::run(void)
{
	m_transferMain.createLocalServerNode();
	std::cout << "Server Name:" << m_transferMain.getServerQueueName() << std::endl << std::endl;

	while(true)
	{
		m_transferMain.receive();

		switch(m_transferMain.getMessageType())
		{
		case CSMTransfer::MsgType::ask:
			std::cout << "Request: " << m_transferMain.getFileName() << " from " << m_transferMain.getClientQueueName() << std::endl;
			dispatchNewTransfer();
			break;
		case CSMTransfer::MsgType::terminate:
			return;
		}
	}
}

void CSMServer::launch(void)
{
	if(m_transferMain.isNameServerAvailable() == false)
	{
		std::cout << "Couldn't connect to COM NameServer" << std::endl;
		return;
	}

	// create directory for received files
	try {
		m_errorMsg = "Couldn't create directory";
		fs::create_directory(fs::path(m_recvdDirName));
	}
	catch(fs::filesystem_error& e) {
		std::cout << m_errorMsg << " : " << e.what() << std::endl;
		return;
	}

	try {
		run();	// main loop
	}
	catch(boost::interprocess::interprocess_exception& e) {
		std::cout << e.what() << std::endl;
		return;
	}
}



int _tmain(int argc, _TCHAR* argv[])
{
	::SetConsoleOutputCP(::GetACP());

	// setup options
	opt::options_description desc("Options");
	desc.add_options()
		("help,?", "Display this help message")
		("start,R", "Start server")
		("stop,S", "Stop running server instance")
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
	if(vmap.count("stop"))
	{
		// find running server and send terminate message
		CSMTransfer msg;
		msg.createRemoteServerNode();
		std::cout << "Server name: " << msg.getServerQueueName() << std::endl;
		msg.msgMakeTerminate();
		msg.send();

		return 0;
	}
	if(vmap.count("start"))
	{
		CSMServer smServer;
		smServer.launch();
	}

	return 0;
}

