/*
    Copyright (C) 2015 by Project Tox <https://tox.im>

    This file is part of qTox, a Qt-based graphical interface for Tox.

    This program is libre software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

    See the COPYING file for more details.
*/

#ifndef PROFILE_H
#define PROFILE_H

#include <QThread>
#include "toxcore.h"

class ToxAv;
class LocalFileEncryptor;
class Friend;
class Group;
template<class K, class V> class QHash;

/* No other class should call any Profile->tox or Profile->toxav
   functions directly, due to threading issues. Instead emit a signal and connect
   that to whatever slot on ToxCore/ToxAv. */

class Profile : public QObject
{
    Q_OBJECT

public:
    Profile(const QString& profile);

    ToxCore* tox; // this object lives in the Nexus::coreThread
    ToxAv* toxav; // this object lives in the Nexus::avThread
    LocalFileEncryptor* encmgr;

    void saveConfiguration();

signals:
    void badProxy();
    void failedToStart();

private:
    QThread coreThread;
    QThread avThread;

    QHash<int, Friend> friendsList;
    QHash<int, Group*> groupList;
};


#endif // PROFILE_H
