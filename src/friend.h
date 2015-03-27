/*
    Copyright (C) 2014 by Project Tox <https://tox.im>

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

#ifndef FRIEND_H
#define FRIEND_H

#include <QObject>
#include <QString>
#include "corestructs.h"
#include "profile.h"

struct FriendWidget;
class ChatForm;

class Friend : public QObject
{
    Q_OBJECT
public:
    Friend(Profile* profile);
    Friend(const Friend& other)=delete;
    ~Friend();
    Friend& operator=(const Friend& other)=delete;

    void setAlias(QString name);
    QString getDisplayedName() const;

    void setEventFlag(int f);
    int getEventFlag() const;

    ChatForm *getChatForm();
    FriendWidget *getFriendWidget();

signals:
    void displayedNameChanged(FriendWidget* widget, Status s, int hasNewEvents);

private:
    Profile* profile
    QString userAlias;
    int hasNewEvents;
    FriendWidget* widget;
    ChatForm* chatForm;
};

#endif // FRIEND_H
