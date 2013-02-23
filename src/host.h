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
#ifndef HOST_H
#define HOST_H

#include <QString>
#include <QList>

class Host
{
public:
    Host(const QString&, const QString&, const QString&);
    ~Host() {};
    QString host() const { return h; };
    QString loginName() const { return lg; };
    QString password() const { return pw; };
    // operators <, ==
    int operator== (Host h)
        { return (this->host() == h.host() && this->loginName() == h.loginName()); };
    int operator== (Host *h)
        { return (this->host() == h->host() && this->loginName() == h->loginName()); };
    /* Do we need any of these anymore? */
    /*bool operator< (Host h)
        { return (this->title() < h.title()); };
    bool operator< (Host *h)
        { return (this->title() < h->title()); };*/

private:
    QString h;
    QString lg;
    QString pw;
};

class HostList : public QList<Host*>
{
public:
    HostList() {};
    virtual ~HostList() {};

    virtual int compareItems(Host * d1, Host * d2);
};

#endif // HOST_H
