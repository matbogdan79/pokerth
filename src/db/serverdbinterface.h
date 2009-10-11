/***************************************************************************
 *   Copyright (C) 2009 by Lothar May                                      *
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
/* Server database interface. */

#ifndef _SERVERDBINTERFACE_H_
#define _SERVERDBINTERFACE_H_

#include <db/serverdbcallback.h>
#include <string>
#include <list>

typedef std::list<DB_id> db_list;

class ServerDBInterface
{
public:
	virtual ~ServerDBInterface();

	virtual void Init(const std::string &host, const std::string &user, const std::string &pwd,
					  const std::string &database, const std::string &encryptionKey) = 0;

	virtual void Start() = 0;
	virtual void Stop() = 0;

	virtual void AsyncPlayerLogin(unsigned requestId, const std::string &playerName) = 0;
	virtual void PlayerLogout(DB_id playerId) = 0;

	virtual void AsyncCreateGame(unsigned requestId, const std::string &gameName) = 0;
	virtual void SetGamePlayerPlace(DB_id gameId, DB_id playerId, unsigned place) = 0;
	virtual void EndGame(DB_id gameId) = 0;
};

#endif
