#pragma once

#include <cstdint>
#include <string>

#include <boost/serialization/access.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace ipc {
	using namespace boost::interprocess;
};

#include "SMNameServer.h"


class CSMTransfer
{
	friend class boost::serialization::access;

public:
	enum class MsgType : unsigned int {
		empty,
		ask,
		allow,
		deny,
		transfer,
		proceed,
		terminate,
	};
	static const unsigned int max_message_size = 4096;

private:
	std::shared_ptr<CSMNameServer> m_SMNameServer;
	std::shared_ptr<ipc::message_queue> m_MQLocal;
	std::shared_ptr<ipc::message_queue> m_MQRemote;
	std::shared_ptr<ipc::shared_memory_object> m_sharedMemory;
	std::shared_ptr<ipc::file_mapping> m_mappedFile;

	class data
	{
	public:
		CSMTransfer::MsgType m_msgType;
		std::string m_serverQueueName;
		std::string m_clientQueueName;
		std::string m_serverSharedMemoryName;
		std::string m_filePath;
		std::string m_fileName;
		std::uintmax_t m_fileSize;
		std::uintmax_t m_fileOffset;
		std::uintmax_t m_remainderSize;
		std::size_t m_regionSize;
		std::size_t m_copySize;

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar & m_msgType;
			switch(m_msgType)
			{
			case CSMTransfer::MsgType::empty:		// (not for interchange)
				break;
			case CSMTransfer::MsgType::ask:			// client
				ar & m_fileSize;
				ar & m_fileName;
				ar & m_fileOffset;	// just initializing server-side class member with 0
				ar & m_clientQueueName;
				break;
			case CSMTransfer::MsgType::allow:		// server (reply to "ask")
				ar & m_regionSize;
				ar & m_serverQueueName;
				ar & m_serverSharedMemoryName;
				break;
			case CSMTransfer::MsgType::deny:		// server (reply to "ask")
				break;
			case CSMTransfer::MsgType::transfer:	// client
				break;
			case CSMTransfer::MsgType::proceed:		// server (reply to "transfer")
				break;
			case CSMTransfer::MsgType::terminate:	// server->server (shutdown)
				break;
			}
		}
	} m_data;

	void createNode(std::string* localQueue, std::string* remoteQueue); // throws ipc::interprocess_exception
	bool regionCopyToFrom(ipc::mapped_region& rgnTo, ipc::mapped_region& rgnFrom);
	void updateCopySize(void);

public:
	CSMTransfer(void);
	~CSMTransfer(void);

	bool isNameServerAvailable(void);

	void createLocalServerNode(void); // throws ipc::interprocess_exception
	void createRemoteServerNode(void); // throws ipc::interprocess_exception
	void createClientToServerNode(std::string clientFilePath); // throws ipc::interprocess_exception
	void createServerToClientNotifyNode(CSMTransfer& localServerNode); // throws ipc::interprocess_exception
	void createServerToClientTransferNode(CSMTransfer& localServerNode); // throws ipc::interprocess_exception
	void createClientToServerTransferNode(CSMTransfer& clientToServerNode); // throws ipc::interprocess_exception
	void updateClientToServerNode(void); // throws ipc::interprocess_exception

	void createClientMemoryObjects(void); // throws ipc::interprocess_exception , fs::filesystem_error
	void createServerMemoryObjects(std::string serverFilePath); // throws ipc::interprocess_exception

	bool regionSend(void);
	bool regionReceive(void);

	void send(void);    // throws ipc::interprocess_exception , boost::archive::archive_exception
	void receive(void); // throws ipc::interprocess_exception , boost::archive::archive_exception
	void sendAndReceive(); // throws ipc::interprocess_exception , boost::archive::archive_exception

	// initializers for each message type
	void msgMakeEmpty(void);
	void msgMakeAsk(std::string fileName);
	void msgMakeAllow(std::size_t blockSize);
	void msgMakeDeny(void);
	void msgMakeTransfer(void);
	void msgMakeProceed(void);
	void msgMakeTerminate(void);

	CSMTransfer::MsgType getMessageType(void);
	std::string& getServerQueueName(void);
	std::string& getClientQueueName(void);
	std::string& getFileName(void);
	std::uintmax_t getFileSize(void);
	std::uintmax_t getFileOffset(void);
};




