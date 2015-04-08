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

/* A C++11/Qt5 thin wrapper to the Tox toxcore public API in tox.h.
 * This is a *thin* wrapper, anyone using this is expected to have also
 * read toxcore's header, linked at the bottom of this comment.
 *
 * Having said that, these are some things to know about this wrapper:
 * Since the errors exposed by the core API are largely internal errors,
 * best used for debugging, or external errors that are preventable in clients
 * such that their presence is a bug in the client, it is a design choice that
 * the errors are ignored other than being logged to file, and instead the
 * caller will get an invalid return value (e.g. UINT32_MAX, QString.isNull())
 * In some cases the return value is an enum without an INVALID or similar
 * member of the enum; in such cases the return type is QVariant, which will be
 * QVariant::isNull() on error, or if not, then use QVariant::toInt() and cast
 * to the enum type.
 * Some changes have been made for consistency with Qt conventions. The changes
 * themselves should also be consistent across the API.
 * Note: in some cases, an empty string is valid, so be sure to use isNull()
 * and _not_ isEmpty().
 *
 * Functions are named the same, except in camelCase
 * self_get_nospam becomes getSelfNoSpam, for consistency with Qt conventions
 * Error enums are ignored except for logging purposes
 *   - Return values should instead be tested for success
 * uint8_t arrays for data are converted to/from QByteArrays, and
 * uint8_t arrays for text are converted to/from QStrings
 * Functions that fill an array whose size is provided by another function are wrapped into one,
 *     using the QByteArray/QString objects to handle sizing automatically
 *
 * Otherwise, the documentation from tox.h applies for all functions here
 * https://github.com/irungentoo/toxcore/blob/master/toxcore/tox.h */

#ifndef QTOXCORE_H
#define QTOXCORE_H

#include <tox/tox.h>
#include <QByteArray>
#include <QString>
#include <QList>
#include <QVariant>
#include "corestructs.h"

class ToxOptions
{
public:
    ToxOptions(); // gives default options
    ~ToxOptions();

    // clears this and sets it to default opts
    void optionsDefault();

    Tox_Options* tox_options = nullptr;
};

class ToxAv;

#define TOXCORE_MAX_AVATAR_LENGTH (32 * 1<<10)
// 32 KiB for now: TODO

class ToxCore : public QObject
{
    Q_OBJECT

    friend class ToxAv; // used in accessing Tox* tox for toxav_new

public:
    // the constructor wraps tox_new: data is any save data, null/empty data for new profile
    ToxCore(const ToxOptions& options = ToxOptions(), const QByteArray& data = QByteArray());
    // check any errors from the constructor, such as bad proxy
    TOX_ERR_NEW constructorError() const;
    ~ToxCore();

    // gets the save data to be written to file
    QByteArray getSaveData() const;

    // returns true on success
    bool bootstrap(const QString& host, uint16_t port, const QByteArray& publicKey);

    // like bootstrap(), but for tcp relays
    bool addTcpRelay(const QString& host, uint16_t port, const QByteArray& publicKey);

    // returns if and how are connected to the DHT
    // this should be avoided in preference to the corresponding signal
    TOX_CONNECTION getSelfConnectionStatus() const;

    // ms before calling iterate()
    uint32_t iterationInterval() const;

    // the main loop, called as iterationInterval dictates
    void iterate();


    /* BEGIN SELF PSEUDO NAMESPACE */

    // get your own Tox friend address
    QByteArray getSelfAddress() const;

    // get your own nospam
    uint32_t getSelfNoSpam() const;

    // set your own nospam
    void setSelfNoSpam(uint32_t nospam);

    // get your own public key
    QByteArray getSelfPublicKey() const;

    // get your own secret key
    QByteArray getSelfSecretKey() const;

    // get your own nickname
    QString getSelfName() const;

public slots:
    // set your own nickname, returns true on success (limited to TOX_MAX_NAME_LENGTH)
    bool setSelfName(const QString& name);

public:
    // get your own status message
    QString getSelfStatusMessage() const;

public slots:
    // set your own status message, returns true on success
    bool setSelfStatusMessage(const QString& status);

public:
    // get your user status (online, busy, away)
    TOX_USER_STATUS getSelfStatus() const;

    // set your user status (online, busy, away)
    void setSelfStatus(TOX_USER_STATUS user_status);


    /* BEGIN FRIEND PSEUDO NAMESPACE */

    // Add a friend, message required. INT32_MAX is the maximum number of friends
    // returns the friend number, or UINT32_MAX on failure
    uint32_t friendAdd(const QByteArray& address, const QString& message);

    // (attempts to) connect to the friend without sending a friend request
    // also returns the friendnumber/UINT32_MAX
    uint32_t friendAddNoRequest(const QByteArray& public_key);

    // (silently) removes a friend, returns true on success
    bool friendDelete(uint32_t friend_number);

    // translates a friend's pk to their current friend_number, returns UINT32_MAX on failure
    uint32_t friendByPublicKey(const QByteArray& public_key) const;

    // get the pk for the given friend_number, or null QBA on error
    QByteArray friendGetPublicKey(uint32_t friend_number) const;

    // check if friend_number is valid
    bool friendExists(uint32_t friend_number) const;

    // get a list of friend_numbers (kinda exists in both namespaces)
    QList<uint32_t> getSelfFriendList() const;

    // get the friend's nickname, returns isNull() string on error
    // note: isEmpty() strings are valid nicks, but not isNull()
    // should probably use the corresponding signal instead of this
    QString friendGetName(uint32_t friend_number) const;

    // get the friend's status message, returns isNull() string on error
    // note: isEmpty() strings are valid nicks, but not isNull()
    // should probably use the corresponding signal instead of this
    QString friendGetStatusMessage(uint32_t friend_number) const;

    // get friend's user status (online, away, busy)
    // use QVariant::toInt and cast to TOX_USER_STATUS enum
    // should probably use the corresponding signal instead of this
    QVariant friendGetStatus(uint32_t friend_number) const;

    // get friend's connection status (none, tcp, or udp)
    // use QVariant::toInt and cast to TOX_CONNECTION enum
    // should probably use the corresponding signal instead of this
    QVariant friendGetConnectionStatus(uint32_t friend_number) const;

    // get whether or not the friend is typing
    // use QVariant::toBool
    // should probably use the corresponding signal instead of this
    QVariant friendGetTyping(uint32_t friend_number) const;

    // set whether or not we are typing, returns true on success
    bool selfSetTyping(uint32_t friend_number, bool is_typing);

    // send a message not exceeding TOX_MAX_MESSAGE_LENGTH, of TOX_MESSAGE_TYPE
    // returns QVariant holding the message id, or null
    // use QVariant::value<uint32_t>() to get the message id
    QVariant friendSendMessage(uint32_t friend_number, TOX_MESSAGE_TYPE type, const QString& message);


    /* BEGIN FILE PSEUDO NAMESPACE */

    // get a cryptographic hash, returns null QBA on error
    static QByteArray hash(const QByteArray& data);

    // send a file control command, returns true on success
    bool fileControl(uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control);

    // send a file seek command, used only for resuming, returns true on success
    bool fileSeek(uint32_t friend_numner, uint32_t file_number, uint64_t position);

    // get the unique and persistent file_id for a transfer, returns null QBA on error
    QByteArray fileGetFileID(uint32_t friend_number, uint32_t file_number) const;

    // send a file transfer request. Max name length TOX_MAX_FILENAME_LENGTH,
    // kind TOX_FILE_KIND (or custom extension)
    // you may optionally set your own file_id (TOX_FILE_ID_LENGTH), which is
    // unique and persistent and sometimes has meaning regarding conditional
    // transfers
    // returns UINT32_MAX on any error
    uint32_t fileSend(uint32_t friend_number, uint32_t kind, uint64_t file_size, const QByteArray& file_id, const QString& filename);

    // sends a chunk of data over a file transfer, to be called in response to
    // the fileChunkRequested signal. returns true on success
    bool fileSendChunk(uint32_t friend_number, uint32_t file_number, uint64_t position, const QByteArray& data);


    /* BEGIN GROUP CHAT NAMESPACE */
    // temporarily for the old groupchat api, some names are inconsistent with the groupThings* namespace

    // create a new groupchat, returns group_number or -1 on failure
    int addGroupchat();

    // delete a group chat, returns true on success
    bool delGroupchat(int group_number);

    // get a peer's name
    QString groupPeerName(int group_number, int peer_number) const;

    // get a peer's Tox ID pubkey
    QByteArray groupPeerPubkey(int group_number, int peer_number) const;

    // invite a friend to a group, returns true on success
    bool inviteFriend(int32_t friend_number, int group_number);

    // join a group you've been invited to, returns group_number or -1 on failure
    int joinGroupchat(int32_t friend_number, const QByteArray& data);

    // send a message to a group, returns success
    bool groupMessageSend(int group_number, const QString& message);

    // send an action to a group, returns success
    bool groupActionSend(int group_number, const QString& action);

    // set a group's title, returns success
    bool groupSetTitle(int group_number, const QString& title);

    // get a group's title
    QString groupGetTitle(int group_number); // const

    // check if the peernumber is ours
    bool groupPeernumberIsOurs(int group_number, int peer_number) const;

    // get the number of peers, returns -1 on failure
    int groupNumberPeers(int group_number) const;

    // get a list of peers in a chat
    QStringList groupGetNames(int group_number) const;

    // there are two functions in the old group chat api that I'm
    // omitting because I don't foresee any use for them in qTox

    // get if av enabled groupchat (TOX_GROUPCHAT_TYPE), returns -1 on failure
    int groupGetType(int group_number) const;


    /* BEGIN EXTRA FUNCTIONS */

    // send a custom packet, returns true on success
    bool friendSendLossyPacket(uint32_t friend_number, const QByteArray& data);

    bool friendSendLosslessPacket(uint32_t friend_number, const QByteArray& data);

    // get your DHT pk (different from Tox ID pk)
    QByteArray getSelfDhtID() const;

    // get the port we're bound to, 0 on error
    uint16_t getSelfUdpPort() const;

    // get the TCP port we're bound to, if acting as a TCP relay. 0 on error
    uint16_t getSelfTcpPort() const;

signals:
    // DHT connection status. Read the tox.h documentation on this callback,
    // handle the signal with care.
    void selfConnectionStatusChanged(TOX_CONNECTION connection_status);

    // friend's name changed
    void friendNameChanged(uint32_t friend_number, QString name);

    // friend's status message changed
    void friendStatusMessageChanged(uint32_t friend_number, QString message);

    // friend's status changed
    void friendStatusChanged(uint32_t friend_number, TOX_USER_STATUS status);

    // friend's connection status changed
    void friendConnectionStatusChanged(uint32_t friend_number, TOX_CONNECTION connection_status);

    // friend's typing status changed
    void friendTypingChanged(uint32_t friend_number, bool is_typing);

    // friend sent a read receipt
    void friendReadReceiptReceived(uint32_t friend_number, uint32_t message_id);

    // someone sent a friend request
    void friendRequestReceived(QByteArray public_key, QString message);

    // friend sent a message
    void friendMessageReceived(uint32_t friend_number, TOX_MESSAGE_TYPE type, QString message);

    // friend sent a file control
    void fileControlReceived(uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control);

    // friend requested a file chunk. Note the reversed word order of the name
    void fileChunkRequested(uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length);

    // friend wants to send us a file
    void fileReceiveRequested(uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, QString filename);

    // friend has sent a chunk
    void fileChunkReceived(uint32_t friend_number, uint32_t file_number, uint64_t position, QByteArray data);

    // a friend sent us a group invite of TOX_GROUPCHAT_TYPE_{TEXT,AV}
    void groupInviteReceived(int32_t friend_number, uint8_t type, QByteArray data);

    // group sent a message
    void groupMessageReceived(int group_number, int peer_number, QString message);

    // group sent an action
    void groupActionReceived(int group_number, int peer_number, QString action);

    // group title has changed
    void groupTitleChanged(int group_number, int peer_number, QString title);

    // group peers have changed
    void groupNamelistChanged(int group_number, int peer_number, TOX_CHAT_CHANGE change);

    // friend sent a custom packet
    void friendLossyPacketReceived(uint32_t friend_number, QByteArray data);

    // friend sent a custom packet
    void friendLosslessPacketReceived(uint32_t friend_number, QByteArray data);

    // one custom signal for the GUI, emitted when this is constructed
    void loaded(QByteArray address);

signals: // dummy signals for how the ui currently operates. These should be removed. TODO
    void usernameSet(QString name);
    void statusMessageSet(QString msg);

private:
    Tox* tox;
    TOX_ERR_NEW error;
};

#endif // QTOXCORE_H
