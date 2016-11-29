#include "stdafx.h"
#include "SMTransfer.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/filesystem.hpp>

namespace fs {
	using namespace boost::filesystem;
};


CSMTransfer::CSMTransfer(void)
	: m_SMNameServer(new CSMNameServer)
{
}

CSMTransfer::~CSMTransfer(void)
{
}


bool CSMTransfer::isNameServerAvailable(void)
{
	if(m_SMNameServer == nullptr)
	{
		return false;
	}
	return m_SMNameServer->isCreated();
}


// message queue methods

void CSMTransfer::createNode(std::string* localQueue, std::string* remoteQueue)
{
	if(localQueue != nullptr)
	{
		m_MQLocal = std::shared_ptr<ipc::message_queue>(new ipc::message_queue(ipc::create_only, localQueue->c_str(), 1, CSMTransfer::max_message_size));			
	}
	if(remoteQueue != nullptr)
	{
		m_MQRemote = std::shared_ptr<ipc::message_queue>(new ipc::message_queue(ipc::open_only, remoteQueue->c_str()));
	}
}


void CSMTransfer::createLocalServerNode(void)
{
	m_data.m_serverQueueName = m_SMNameServer->getSharedMemoryName();
	createNode(&m_data.m_serverQueueName, nullptr);
}

void CSMTransfer::createRemoteServerNode(void)
{
	m_data.m_serverQueueName = m_SMNameServer->getSharedMemoryName();
	createNode(nullptr, &m_data.m_serverQueueName);
}

void CSMTransfer::createClientToServerNode(std::string clientFilePath)
{
	m_data.m_filePath = clientFilePath;
	m_data.m_fileSize = fs::file_size(m_data.m_filePath);

	m_data.m_serverQueueName = m_SMNameServer->getSharedMemoryName();
	m_data.m_clientQueueName = m_SMNameServer->getRandomName();
	createNode(&m_data.m_clientQueueName, &m_data.m_serverQueueName);
}


void CSMTransfer::createServerToClientNotifyNode(CSMTransfer& localServerNode)
{
	// just to notify client, no response allowed
	m_data.m_clientQueueName = localServerNode.m_data.m_clientQueueName;
	createNode(nullptr, &m_data.m_clientQueueName);
}

void CSMTransfer::createServerToClientTransferNode(CSMTransfer& localServerNode)
{
	// copy all info given by client & set another server queue
	m_data = localServerNode.m_data;
	m_data.m_serverQueueName = m_SMNameServer->getRandomName();
	createNode(&m_data.m_serverQueueName, &m_data.m_clientQueueName);
}

void CSMTransfer::createClientToServerTransferNode(CSMTransfer& clientToServerNode)
{
	// copy all info given by server & open new server queue
	m_data = clientToServerNode.m_data;
	createNode(nullptr, &m_data.m_serverQueueName);
}

void CSMTransfer::updateClientToServerNode(void)
{
	// switch to another server queue
	createNode(nullptr, &m_data.m_serverQueueName);
}


// shared memory & file mapping methods

void CSMTransfer::createClientMemoryObjects(void)
{
	m_sharedMemory = std::shared_ptr<ipc::shared_memory_object>(new ipc::shared_memory_object(ipc::open_only, m_data.m_serverSharedMemoryName.c_str(), ipc::read_write));
	m_mappedFile = std::shared_ptr<ipc::file_mapping>(new ipc::file_mapping(m_data.m_filePath.c_str(), ipc::read_only));

	m_sharedMemory->truncate(m_data.m_regionSize);
}

void CSMTransfer::createServerMemoryObjects(std::string serverFilePath)
{
	m_data.m_filePath = serverFilePath;
	// pre-allocate file (file size is set by client)
	{
		std::filebuf fbuffer;
		fbuffer.open(m_data.m_filePath.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
		fbuffer.pubseekoff(m_data.m_fileSize - 1, std::ios_base::beg);
		fbuffer.sputc(0);
	}

	m_data.m_serverSharedMemoryName = m_SMNameServer->getRandomName();
	m_sharedMemory = std::shared_ptr<ipc::shared_memory_object>(new ipc::shared_memory_object(ipc::create_only, m_data.m_serverSharedMemoryName.c_str(), ipc::read_only));
	m_mappedFile = std::shared_ptr<ipc::file_mapping>(new ipc::file_mapping(m_data.m_filePath.c_str(), ipc::mode_t::read_write));
}


// mapped memory region methods

void CSMTransfer::updateCopySize(void)
{
	m_data.m_remainderSize = m_data.m_fileSize - m_data.m_fileOffset;
	m_data.m_copySize = m_data.m_regionSize;
	if(m_data.m_copySize > m_data.m_remainderSize)
	{
		m_data.m_copySize = static_cast<std::size_t>(m_data.m_remainderSize);
	}
}

bool CSMTransfer::regionCopyToFrom(ipc::mapped_region& rgnTo, ipc::mapped_region& rgnFrom)
{
	bool bContinue = true;
	if(m_data.m_fileOffset < m_data.m_fileSize)
	{
		void* pDst = rgnTo.get_address();
		void* pSrc = rgnFrom.get_address();
		updateCopySize();
		std::memcpy(pDst, pSrc, m_data.m_copySize);
		m_data.m_fileOffset += m_data.m_copySize;
		bContinue = (m_data.m_fileOffset < m_data.m_fileSize);
	}
	return bContinue;
}

bool CSMTransfer::regionSend(void)
{
	updateCopySize();
	ipc::mapped_region regionSharedMemory(*m_sharedMemory, ipc::read_write);
	ipc::mapped_region regionMappedFile(*m_mappedFile, ipc::read_only, m_data.m_fileOffset, m_data.m_copySize);
	bool bContinue = regionCopyToFrom(regionSharedMemory, regionMappedFile);

	msgMakeTransfer();
	send(); // client ->
	receive(); // client <-

	return bContinue;
}

bool CSMTransfer::regionReceive(void)
{
	updateCopySize();
	receive(); // -> server

	ipc::mapped_region regionMappedFile(*m_mappedFile, ipc::read_write, m_data.m_fileOffset, m_data.m_copySize);
	ipc::mapped_region regionSharedMemory(*m_sharedMemory, ipc::read_only);
	bool bContinue = regionCopyToFrom(regionMappedFile, regionSharedMemory);

	msgMakeProceed();
	send(); // <- server

	return bContinue;
}


// serialized message methods

void CSMTransfer::send(void)
{
	// serialize message to string
	std::stringstream streamSend;
	boost::archive::text_oarchive oarc(streamSend);
	oarc << m_data;

	// ...and send it to message queue
	std::string strSend = streamSend.str();
	m_MQRemote->send(strSend.c_str(), strSend.length() + 1, 0);
}

void CSMTransfer::receive(void)
{
	// receive message from queue
	unsigned int priority;
	std::size_t receivedSize;
	std::vector<char> dataBuffer(CSMTransfer::max_message_size);
	char* buffer = dataBuffer.data();
	m_MQLocal->receive(buffer, dataBuffer.size(), receivedSize, priority);

	// ... and deserialize it
	std::string strRecvd(buffer);
	std::stringstream streamRecv(strRecvd);
	boost::archive::text_iarchive iarc(streamRecv);
	iarc >> m_data;
}

void CSMTransfer::sendAndReceive(void)
{
	send();
	receive();
}


// message helper methods

void CSMTransfer::msgMakeEmpty(void)
{
	m_data.m_msgType = CSMTransfer::MsgType::empty;
}

void CSMTransfer::msgMakeAsk(std::string fileName)
{
	m_data.m_msgType = CSMTransfer::MsgType::ask;
	m_data.m_fileName = fileName;
	m_data.m_fileOffset = 0;
}

void CSMTransfer::msgMakeAllow(std::size_t regionSize)
{
	m_data.m_msgType = CSMTransfer::MsgType::allow;
	m_data.m_regionSize = regionSize;
}

void CSMTransfer::msgMakeDeny(void)
{
	m_data.m_msgType = CSMTransfer::MsgType::deny;
}

void CSMTransfer::msgMakeTransfer(void)
{
	m_data.m_msgType = CSMTransfer::MsgType::transfer;
}

void CSMTransfer::msgMakeProceed(void)
{
	m_data.m_msgType = CSMTransfer::MsgType::proceed;
}

void CSMTransfer::msgMakeTerminate(void)
{
	m_data.m_msgType = CSMTransfer::MsgType::terminate;
}


CSMTransfer::MsgType CSMTransfer::getMessageType(void)
{
	return m_data.m_msgType;
}
std::string& CSMTransfer::getServerQueueName(void)
{
	return m_data.m_serverQueueName;
}
std::string& CSMTransfer::getClientQueueName(void)
{
	return m_data.m_clientQueueName;
}
std::string& CSMTransfer::getFileName(void)
{
	return m_data.m_fileName;
}
std::uintmax_t CSMTransfer::getFileSize(void)
{
	return m_data.m_fileSize;
}
std::uintmax_t CSMTransfer::getFileOffset(void)
{
	return m_data.m_fileOffset;
}

