/***************************************************************************
 *   Copyright (C) 2009- by The qGo Project                                *
 *                                                                         *
 *   This file is part of qGo.                                             *
 *                                                                         *
 *   qGo is free software: you can redistribute it and/or modify           *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 *   or write to the Free Software Foundation, Inc.,                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "host.h"

Host::Host(const QString &host, const QString &login, const QString &pass)
    : h(host) , lg(login) , pw(pass)
{
}

/*
 *   List to help keeping things sorted
 */

int HostList::compareItems(Host* d1, Host* d2)
{
    Host *s1 = static_cast<Host*>(d1);
    Host *s2 = static_cast<Host*>(d2);

    Q_CHECK_PTR(s1);
    Q_CHECK_PTR(s2);

    if (s1 > s2)
        return 1;
    else if (s1 < s2)
        return -1;
    else
        // s1 == s2;
        return 0;
}
