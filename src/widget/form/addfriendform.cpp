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

#include "addfriendform.h"

#include <QFont>
#include <QMessageBox>
#include <QErrorMessage>
#include <QClipboard>
#include <tox/tox.h>
#include "ui_mainwindow.h"
#include "src/nexus.h"
#include "src/profile.h"
#include "src/toxdns.h"
#include "src/misc/settings.h"
#include "src/widget/gui.h"

AddFriendForm::AddFriendForm()
{
    main = new QWidget(), head = new QWidget();
    QFont bold;
    bold.setBold(true);
    headLabel.setText(tr("Add Friends"));
    headLabel.setFont(bold);

    toxAddrLabel.setText(tr("Tox ID","Tox ID of the person you're sending a friend request to"));
    messageLabel.setText(tr("Message","The message you send in friend requests"));
    sendButton.setText(tr("Send friend request"));

    main->setLayout(&layout);
    layout.addWidget(&toxAddrLabel);
    layout.addWidget(&toxAddr);
    layout.addWidget(&messageLabel);
    layout.addWidget(&message);
    layout.addWidget(&sendButton);

    head->setLayout(&headLayout);
    headLayout.addWidget(&headLabel);

    connect(&toxAddr,&QLineEdit::returnPressed, this, &AddFriendForm::onSendTriggered);
    connect(&sendButton, SIGNAL(clicked()), this, SLOT(onSendTriggered()));
    connect(Nexus::getProfile()->tox, &ToxCore::usernameSet, this, &AddFriendForm::onUsernameSet); // TODO: core shouldn't be emitting a dummy signal
}

AddFriendForm::~AddFriendForm()
{
    head->deleteLater();
    main->deleteLater();
}

void AddFriendForm::show(Ui::MainWindow &ui)
{
    ui.mainContent->layout()->addWidget(main);
    ui.mainHead->layout()->addWidget(head);
    main->show();
    head->show();
    setAddrFromClipboard();
    toxAddr.setFocus();
}

QString AddFriendForm::getMessage() const
{
    const QString msg = message.toPlainText();
    return !msg.isEmpty() ? msg : message.placeholderText();
}

void AddFriendForm::onUsernameSet(const QString& username)
{
    message.setPlaceholderText(tr("%1 here! Tox me maybe?","Default message in friend requests if the field is left blank. Write something appropriate!").arg(username));
}

void AddFriendForm::onSendTriggered()
{
    QString addr = toxAddr.text().trimmed();

    if (addr.isEmpty())
    {
        GUI::showWarning(tr("Couldn't add friend"), tr("Please fill in a valid Tox ID","Tox ID of the friend you're sending a friend request to"));
    }
    else if (ToxAddr::isToxAddr(addr))
    {
        if (addr.toUpper() == ToxAddr::toHexString(Nexus::getProfile()->tox->getSelfPublicKey())) //TODO thread safety
            GUI::showWarning(tr("Couldn't add friend"), tr("You can't add yourself as a friend!","When trying to add your own Tox ID as friend"));
        else
            emit friendRequested(addr, getMessage());
        this->toxAddr.clear();
        this->message.clear();
    } else {
        if (Settings::getInstance().getProxyType() != ProxyType::ptNone)
        {
            QMessageBox::StandardButton btn = QMessageBox::warning(main, "qTox", tr("qTox needs to use the Tox DNS, but can't do it through a proxy.\n\
Ignore the proxy and connect to the Internet directly?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
            if (btn != QMessageBox::Yes)
                return;
        }

        ToxAddr toxAddr(ToxDNS::resolveToxAddress(addr, true));

        if (toxAddr.toString().isEmpty())
        {
            GUI::showWarning(tr("Couldn't add friend"), tr("This Tox ID does not exist","DNS error"));
            return;
        }

        emit friendRequested(toxAddr.toString(), getMessage());
        this->toxAddr.clear();
        this->message.clear();
    }
}

void AddFriendForm::setAddrFromClipboard()
{
    QClipboard* clipboard = QApplication::clipboard();
    QString addr = clipboard->text().trimmed();
    // TODO COMPILE CHECK
    if (/*Nexus::getProfile()->isReady() &&*/ !addr.isEmpty() && ToxAddr::isToxAddr(addr)) {
        if (addr.toUpper() == ToxAddr::toHexString(Nexus::getProfile()->tox->getSelfPublicKey())) //TODO thread safety
            toxAddr.setText(addr);
    }
}
