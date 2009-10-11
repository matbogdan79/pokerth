/***************************************************************************
 *   Copyright (C) 2007 by Lothar May                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <net/sessiondata.h>
#include <gsasl.h>

using namespace std;

SessionData::SessionData(boost::shared_ptr<boost::asio::ip::tcp::socket> sock, SessionId id, SessionDataCallback &cb)
: m_socket(sock), m_id(id), m_gameId(0), m_state(SessionData::Init), m_readyFlag(false),
  m_wantsLobbyMsg(true), m_activityTimeoutNoticeSent(false), m_callback(cb),
  m_authSession(NULL), m_curAuthStep(0)
{
}

SessionData::~SessionData()
{
	m_callback.SignalSessionTerminated(m_id);
	InternalClearAuthSession();
}

SessionId
SessionData::GetId() const
{
	// const value - no mutex needed.
	return m_id;
}

unsigned
SessionData::GetGameId() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_gameId;
}

void
SessionData::SetGameId(unsigned gameId)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_gameId = gameId;
}

SessionData::State
SessionData::GetState() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_state;
}

void
SessionData::SetState(SessionData::State state)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_state = state;
}

boost::shared_ptr<boost::asio::ip::tcp::socket>
SessionData::GetAsioSocket()
{
	return m_socket;
}

bool
SessionData::CreateAuthSession(Gsasl *context, bool server, const string &userName, const string &password)
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_dataMutex);
	InternalClearAuthSession();
	int errorCode;
	if (server)
		errorCode = gsasl_server_start(context, "SCRAM-SHA-1", &m_authSession);
	else
		errorCode = gsasl_client_start(context, "SCRAM-SHA-1", &m_authSession);
	if (errorCode == GSASL_OK)
	{
		gsasl_property_set(m_authSession, GSASL_AUTHID, userName.c_str());
		gsasl_property_set(m_authSession, GSASL_PASSWORD, password.c_str());
		retVal = true;
	}
	return retVal;
}

bool
SessionData::AuthStep(int stepNum, const std::string &inData, std::string &outData)
{
	bool retVal = false;
	boost::mutex::scoped_lock lock(m_dataMutex);
	if (m_authSession && stepNum == m_curAuthStep + 1)
	{
		m_curAuthStep = stepNum;
		char *tmpOut;
		size_t tmpOutSize;
		int errorCode = gsasl_step(m_authSession, inData.c_str(), inData.length(), &tmpOut, &tmpOutSize);
		if (errorCode == GSASL_NEEDS_MORE)
		{
			outData = string(tmpOut, tmpOutSize);
			retVal = true;
		}
		else if (errorCode == GSASL_OK && stepNum != 1)
		{
			outData = string(tmpOut, tmpOutSize);
			retVal = true;
			InternalClearAuthSession();
		}
		gsasl_free(tmpOut);
	}
	return retVal;
}

void
SessionData::InternalClearAuthSession()
{
	if (m_authSession)
	{
		gsasl_finish(m_authSession);
		m_authSession = NULL;
		m_curAuthStep = 0;
	}
}

void
SessionData::SetReadyFlag()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_readyFlag = true;
}

void
SessionData::ResetReadyFlag()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_readyFlag = false;
}

bool
SessionData::IsReady() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_readyFlag;
}

void
SessionData::SetWantsLobbyMsg()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_wantsLobbyMsg = true;
}

void
SessionData::ResetWantsLobbyMsg()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_wantsLobbyMsg = false;
}

bool
SessionData::WantsLobbyMsg() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_wantsLobbyMsg;
}

const std::string &
SessionData::GetClientAddr() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_clientAddr;
}

void
SessionData::SetClientAddr(const std::string &addr)
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_clientAddr = addr;
}

ReceiveBuffer &
SessionData::GetReceiveBuffer()
{
	// mutex protection, if needed, within buffer.
	return m_receiveBuffer;
}

void
SessionData::ResetActivityTimer()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_activityTimeoutNoticeSent = false;
	m_activityTimer.reset();
	m_activityTimer.start();
}

unsigned
SessionData::GetActivityTimerElapsedSec() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_activityTimer.elapsed().total_seconds();
}

bool
SessionData::HasActivityNoticeBeenSent() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_activityTimeoutNoticeSent;
}

void
SessionData::MarkActivityNotice()
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	m_activityTimeoutNoticeSent = true;
}

unsigned
SessionData::GetAutoDisconnectTimerElapsedSec() const
{
	boost::mutex::scoped_lock lock(m_dataMutex);
	return m_autoDisconnectTimer.elapsed().total_seconds();
}
