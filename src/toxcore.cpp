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

#include "toxcore.h"
#include "misc/cstring.h"
#include <new>
#include <QDebug>

/* First, some utility functions for reducing code duplication, and macros
   to simplify their usage. Thank fscking god for variadic templates.
*/

/* Use a nifty template to heavily reduce code duplication.
   It wraps any core function with an error checker/logger
   (itself wrapped in a macro to get the function name for the error log).
   Sadly it can't be used for functions with undefined retval on error, since the
   caller doesn't see any errors.
   Many thanks to nurupo and iphy for much help with the templating
*/

template<typename Func, typename E, typename ...Args>
typename std::result_of<Func (Args..., E *)>::type qtox_call_core(const char* name, Func func, E ok_val, Args ...args)
{
    E error;
    auto ret = func(args..., &error);
    if (error != ok_val)
        qWarning() << "Error:" << name << "failed with code" << error;
    return ret;
}
#define QTOX_CALL_CORE(FUNC, OK, ...) qtox_call_core(#FUNC, FUNC, OK, __VA_ARGS__)

template <typename Func, typename E, typename ...ARGS>
static QVariant qtox_call_core_variant(const char* name, Func func, E ok_val, ARGS ...args)
{
    E error;
    auto ret = func(args..., &error);
    if (error != ok_val)
    {
        qWarning() << "Error:" << name << "failed with code" << error;
        return QVariant();
    }
    else
    {
        return QVariant(ret);
    }
}
#define QTOX_CALL_CORE_VARIANT(FUNC, OK, ...) qtox_call_core_variant(#FUNC, FUNC, OK, __VA_ARGS__)

typedef enum {CLAMP_LTE, CLAMP_EQ, CLAMP_GTE} CLAMP_TYPE;
static bool clamp(size_t sz, size_t req, CLAMP_TYPE type, const char* func, int line)
{
    if      (type == CLAMP_LTE && sz <= req)
            return true;
    else if (type == CLAMP_EQ  && sz == req)
            return true;
    else if (type == CLAMP_GTE && sz >= req)
            return true;
    else
        qWarning() << "Error: unknown clamp type";

    qWarning() << "Error:" << func << "failed a clamp at line" << line << "sizes:" << sz << req;
    return false;
}
#define CLAMP(sz, req, type) clamp(sz, req, type, __func__, __LINE__)
// C++11 defines __func__ (as a local variable) and __LINE__ (a macro)
// To make best use of this, only allow one call per line

// I'm not copying and pasting this, much less typing it each time
#define  QBA_CAST(data) (reinterpret_cast<const char*>   (data))
#define  UI8_CAST(data) (reinterpret_cast<      uint8_t*>(data))
#define CUI8_CAST(data) (reinterpret_cast<const uint8_t*>(data))

// ditto
#define CORE_CAST(ptr) (static_cast<ToxCore*>((ptr)))

////////////////////////////////////////////////////////////////////////////////
// Now the implementation

ToxOptions::ToxOptions()
{
    tox_options = tox_options_new(nullptr); // nullptr on malloc failure
    if (!tox_options) // emulate new operator
    {
        qWarning() << "Amazing. You actually broke malloc.";
        std::bad_alloc exception;
        throw exception;
    }
}

ToxOptions::~ToxOptions()
{
    tox_options_free(tox_options);
}

// clears this and sets it to default opts
void ToxOptions::optionsDefault()
{
    tox_options_default(tox_options);
}

QByteArray ToxCore::getSaveData() const
{
    QByteArray ret;
    ret.resize(tox_get_savedata_size(tox));
    tox_get_savedata(tox, UI8_CAST(ret.data()));
    return ret;
}

bool ToxCore::bootstrap(const QString& host, uint16_t port, const QByteArray& publicKey)
{
    if (   !CLAMP(host.size(), 255, CLAMP_LTE)
        || !CLAMP(publicKey.size(), TOX_PUBLIC_KEY_SIZE, CLAMP_EQ))
            return false;
    CString hst(host);
    return QTOX_CALL_CORE(tox_bootstrap, TOX_ERR_BOOTSTRAP_OK, tox, reinterpret_cast<const char*>(hst.data()), port, CUI8_CAST(publicKey.data()));
}

bool ToxCore::addTcpRelay(const QString& host, uint16_t port, const QByteArray& publicKey)
{
    if (   !CLAMP(host.size(), 255, CLAMP_LTE)
        || !CLAMP(publicKey.size(), TOX_PUBLIC_KEY_SIZE, CLAMP_EQ))
            return false;
    CString hst(host);
    return QTOX_CALL_CORE(tox_add_tcp_relay, TOX_ERR_BOOTSTRAP_OK, tox, reinterpret_cast<const char*>(hst.data()), port, CUI8_CAST(publicKey.data()));
}

TOX_CONNECTION ToxCore::getSelfConnectionStatus() const
{
    return tox_self_get_connection_status(tox);
}

static void _selfConnectionStatusChanged(Tox*, TOX_CONNECTION status, void* data)
{
    emit CORE_CAST(data)->selfConnectionStatusChanged(status);
}

uint32_t ToxCore::iterationInterval() const
{
    return tox_iteration_interval(tox);
}

void ToxCore::iterate()
{
    tox_iterate(tox);
}

QByteArray ToxCore::getSelfAddress() const
{
    QByteArray ret;
    ret.resize(TOX_ADDRESS_SIZE);
    tox_self_get_address(tox, UI8_CAST(ret.data()));
    return ret;
}

uint32_t ToxCore::getSelfNoSpam() const
{
    return tox_self_get_nospam(tox);
}

void ToxCore::setSelfNoSpam(uint32_t nospam)
{
    tox_self_set_nospam(tox, nospam);
}

QByteArray ToxCore::getSelfPublicKey() const
{
    QByteArray ret;
    ret.resize(TOX_PUBLIC_KEY_SIZE);
    tox_self_get_public_key(tox, UI8_CAST(ret.data()));
    return ret;
}

QByteArray ToxCore::getSelfSecretKey() const
{
    QByteArray ret;
    ret.resize(TOX_SECRET_KEY_SIZE);
    tox_self_get_secret_key(tox, UI8_CAST(ret.data()));
    return ret;
}

QString ToxCore::getSelfName() const
{
    QByteArray ret;
    ret.resize(tox_self_get_name_size(tox));
    tox_self_get_name(tox, UI8_CAST(ret.data()));
    return QString(ret);
}

bool ToxCore::setSelfName(const QString& name)
{
    bool ret;
    if (name.isEmpty())
    {
        ret = QTOX_CALL_CORE(tox_self_set_name, TOX_ERR_SET_INFO_OK, tox, nullptr, 0);
    }
    else
    {
        CString str(name);
        if (!CLAMP(str.size(), TOX_MAX_NAME_LENGTH, CLAMP_LTE))
            return false;
        ret = QTOX_CALL_CORE(tox_self_set_name, TOX_ERR_SET_INFO_OK, tox, str.data(), str.size());
    }
    if (ret)
        emit usernameSet(name); // TODO: remove
    return ret;
}

QString ToxCore::getSelfStatusMessage() const
{
    QByteArray ret;
    ret.resize(tox_self_get_status_message_size(tox));
    tox_self_get_status_message(tox, UI8_CAST(ret.data()));
    return QString(ret);
}

bool ToxCore::setSelfStatusMessage(const QString& status)
{
    bool ret;
    if (status.isEmpty())
    {
        ret = QTOX_CALL_CORE(tox_self_set_status_message, TOX_ERR_SET_INFO_OK, tox, nullptr, 0);
    }
    else
    {
        CString str(status);
        if (!CLAMP(str.size(), TOX_MAX_STATUS_MESSAGE_LENGTH, CLAMP_LTE))
            return false;
        ret = QTOX_CALL_CORE(tox_self_set_status_message, TOX_ERR_SET_INFO_OK, tox, str.data(), str.size());
    }
    if (ret)
        emit statusMessageSet(status); // TODO: remove
    return ret;
}

TOX_USER_STATUS ToxCore::getSelfStatus() const
{
    return tox_self_get_status(tox);
}

void ToxCore::setSelfStatus(TOX_USER_STATUS user_status)
{
    tox_self_set_status(tox, user_status);
}

uint32_t ToxCore::friendAdd(const QByteArray& address, const QString& message)
{
    if (!CLAMP(message.size(), 1, CLAMP_GTE))
        return UINT32_MAX;
    CString msg(message);
    if (   !CLAMP(msg.size(), TOX_MAX_FRIEND_REQUEST_LENGTH, CLAMP_LTE)
        || !CLAMP(address.size(), TOX_ADDRESS_SIZE, CLAMP_EQ))
            return UINT32_MAX;
    return QTOX_CALL_CORE(tox_friend_add, TOX_ERR_FRIEND_ADD_OK, tox, CUI8_CAST(address.data()), msg.data(), msg.size());
}

uint32_t ToxCore::friendAddNoRequest(const QByteArray& publicKey)
{
    if (!CLAMP(publicKey.size(), TOX_PUBLIC_KEY_SIZE, CLAMP_EQ))
        return UINT32_MAX;
    return QTOX_CALL_CORE(tox_friend_add_norequest, TOX_ERR_FRIEND_ADD_OK, tox, CUI8_CAST(publicKey.data()));
}

bool ToxCore::friendDelete(uint32_t friend_number)
{
    return QTOX_CALL_CORE(tox_friend_delete, TOX_ERR_FRIEND_DELETE_OK, tox, friend_number);
}

uint32_t ToxCore::friendByPublicKey(const QByteArray& public_key) const
{
    if (!CLAMP(public_key.size(), TOX_PUBLIC_KEY_SIZE, CLAMP_EQ))
        return UINT32_MAX;
    return QTOX_CALL_CORE(tox_friend_by_public_key, TOX_ERR_FRIEND_BY_PUBLIC_KEY_OK, tox, CUI8_CAST(public_key.data()));
}

QByteArray ToxCore::friendGetPublicKey(uint32_t friend_number) const
{
    QByteArray ret;
    ret.resize(TOX_PUBLIC_KEY_SIZE);
    if (QTOX_CALL_CORE(tox_friend_get_public_key, TOX_ERR_FRIEND_GET_PUBLIC_KEY_OK, tox, friend_number, UI8_CAST(ret.data())))
        return ret;
    else
        return QByteArray();
}

bool ToxCore::friendExists(uint32_t friend_number) const
{
    return tox_friend_exists(tox, friend_number);
}

QList<uint32_t> ToxCore::getSelfFriendList() const
{
    size_t size = tox_self_get_friend_list_size(tox);
    uint32_t* array = new uint32_t[size];
    tox_self_get_friend_list(tox, array);
    QList<uint32_t> ret;
    ret.reserve(size);
    for (size_t i = 0; i < size; i++)
        ret.append(array[i]);
    delete[] array;
    return ret;
}

QString ToxCore::friendGetName(uint32_t friend_number) const
{
    size_t size = QTOX_CALL_CORE(tox_friend_get_name_size, TOX_ERR_FRIEND_QUERY_OK, tox, friend_number);
    if (size == SIZE_MAX)
        return QString();
    QByteArray ret;
    ret.resize(size);
    if (QTOX_CALL_CORE(tox_friend_get_name, TOX_ERR_FRIEND_QUERY_OK, tox, friend_number, UI8_CAST(ret.data())))
        return QString(ret);
    else
        return QString();
}

static void _friendNameChanged(Tox*, uint32_t friend_number, const uint8_t* name, size_t length, void* core)
{
    QString str = CString::toString(name, length);
    emit CORE_CAST(core)->friendNameChanged(friend_number, str);
}

QString ToxCore::friendGetStatusMessage(uint32_t friend_number) const
{
    size_t size = QTOX_CALL_CORE(tox_friend_get_status_message_size, TOX_ERR_FRIEND_QUERY_OK, tox, friend_number);
    if (size == SIZE_MAX)
        return QString();
    QByteArray ret;
    ret.resize(size);
    if (QTOX_CALL_CORE(tox_friend_get_status_message, TOX_ERR_FRIEND_QUERY_OK, tox, friend_number, UI8_CAST(ret.data())))
        return QString(ret);
    else
        return QString();
}

static void _friendStatusMessageChanged(Tox*, uint32_t friend_number, const uint8_t* message, size_t length, void* core)
{
    QString str = CString::toString(message, length);
    emit CORE_CAST(core)->friendStatusMessageChanged(friend_number, str);
}

QVariant ToxCore::friendGetStatus(uint32_t friend_number) const
{
    return QTOX_CALL_CORE_VARIANT(tox_friend_get_status, TOX_ERR_FRIEND_QUERY_OK, tox, friend_number);
}

static void _friendStatusChanged(Tox*, uint32_t friend_number, TOX_USER_STATUS status, void* core)
{
    emit CORE_CAST(core)->friendStatusChanged(friend_number, status);
}

QVariant ToxCore::friendGetConnectionStatus(uint32_t friend_number) const
{
    return QTOX_CALL_CORE_VARIANT(tox_friend_get_connection_status, TOX_ERR_FRIEND_QUERY_OK, tox, friend_number);
}

static void _friendConnectionStatusChanged(Tox*, uint32_t friend_number, TOX_CONNECTION conn, void* core)
{
    emit CORE_CAST(core)->friendConnectionStatusChanged(friend_number, conn);
}

QVariant ToxCore::friendGetTyping(uint32_t friend_number) const
{
    return QTOX_CALL_CORE_VARIANT(tox_friend_get_typing, TOX_ERR_FRIEND_QUERY_OK, tox, friend_number);
}

static void _friendTypingChanged(Tox*, uint32_t friend_number, bool typing, void* core)
{
    emit CORE_CAST(core)->friendTypingChanged(friend_number, typing);
}

bool ToxCore::selfSetTyping(uint32_t friend_number, bool typing)
{
    return QTOX_CALL_CORE(tox_self_set_typing, TOX_ERR_SET_TYPING_OK, tox, friend_number, typing);
}

QVariant ToxCore::friendSendMessage(uint32_t friend_number, TOX_MESSAGE_TYPE type, const QString& message)
{
    CString msg(message);
    if (   !CLAMP(msg.size(), 1, CLAMP_GTE)
        || !CLAMP(msg.size(), TOX_MAX_MESSAGE_LENGTH, CLAMP_LTE))
            return QVariant();
    return QTOX_CALL_CORE_VARIANT(tox_friend_send_message, TOX_ERR_FRIEND_SEND_MESSAGE_OK, tox, friend_number, type, msg.data(), msg.size());
}

static void _friendReadReceiptReceived(Tox*, uint32_t friend_number, uint32_t message_id, void* core)
{
    emit CORE_CAST(core)->friendReadReceiptReceived(friend_number, message_id);
}

static void _friendRequestReceived(Tox*, const uint8_t* public_key, const uint8_t* message, size_t length, void* core)
{
    QByteArray pk(QBA_CAST(public_key), TOX_PUBLIC_KEY_SIZE);
    QString msg = CString::toString(message, length);
    emit CORE_CAST(core)->friendRequestReceived(pk, msg);
}

static void _friendMessageReceived(Tox*, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length, void* core)
{
    QString msg = CString::toString(message, length);
    emit CORE_CAST(core)->friendMessageReceived(friend_number, type, msg);
}

QByteArray ToxCore::hash(const QByteArray& data)
{
    QByteArray ret;
    ret.resize(TOX_HASH_LENGTH);
    if (tox_hash(UI8_CAST(ret.data()), CUI8_CAST(data.data()), data.size()))
        return ret;
    else
        return QByteArray();
}

bool ToxCore::fileControl(uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control)
{
    return QTOX_CALL_CORE(tox_file_control, TOX_ERR_FILE_CONTROL_OK, tox, friend_number, file_number, control);
}

static void _fileControlReceived(Tox*, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void* core)
{
    emit CORE_CAST(core)->fileControlReceived(friend_number, file_number, control);
}

bool ToxCore::fileSeek(uint32_t friend_number, uint32_t file_number, uint64_t position)
{
    return QTOX_CALL_CORE(tox_file_seek, TOX_ERR_FILE_SEEK_OK, tox, friend_number, file_number, position);
}

QByteArray ToxCore::fileGetFileID(uint32_t friend_number, uint32_t file_number) const
{
    QByteArray ret;
    ret.resize(TOX_FILE_ID_LENGTH);
    if (QTOX_CALL_CORE(tox_file_get_file_id, TOX_ERR_FILE_GET_OK, tox, friend_number, file_number, UI8_CAST(ret.data())))
        return ret;
    else
        return QByteArray();
}

uint32_t ToxCore::fileSend(uint32_t friend_number, uint32_t kind, uint64_t file_size, const QByteArray& file_id, const QString& filename)
{
    const uint8_t* id;
    if (file_id.isEmpty())
        id = nullptr;
    else if (!CLAMP(file_id.size(), TOX_FILE_ID_LENGTH, CLAMP_EQ))
        return UINT32_MAX;
    else
        id = CUI8_CAST(file_id.data());
    CString name(filename);
    return QTOX_CALL_CORE(tox_file_send, TOX_ERR_FILE_SEND_OK, tox, friend_number, kind, file_size, id, name.data(), name.size());
}

bool ToxCore::fileSendChunk(uint32_t friend_number, uint32_t file_number, uint64_t position, const QByteArray& data)
{
    return QTOX_CALL_CORE(tox_file_send_chunk, TOX_ERR_FILE_SEND_CHUNK_OK, tox, friend_number, file_number, position, CUI8_CAST(data.data()), data.size());
}

static void _fileChunkRequested(Tox*, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void* core)
{
    emit CORE_CAST(core)->fileChunkRequested(friend_number, file_number, position, length);
}

static void _fileReceiveRequested(Tox*, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t* filename, size_t filename_length, void* core)
{
    QString fname = CString::toString(filename, filename_length);
    emit CORE_CAST(core)->fileReceiveRequested(friend_number, file_number, kind, file_size, fname);
}

static void _fileChunkReceived(Tox*, uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t* data, size_t length, void* core)
{
    QByteArray dat(QBA_CAST(data), length);
    emit CORE_CAST(core)->fileChunkReceived(friend_number, file_number, position, dat);
}

static void _groupInviteReceived(Tox*, int32_t friend_number, uint8_t type, const uint8_t* data, uint16_t length, void* core)
{
    QByteArray dat(QBA_CAST(data), length);
    emit CORE_CAST(core)->groupInviteReceived(friend_number, type, dat);
}

static void _groupMessageReceived(Tox*, int group_number, int peer_number, const uint8_t* message, uint16_t length, void* core)
{
    QString msg = CString::toString(message, length);
    emit CORE_CAST(core)->groupMessageReceived(group_number, peer_number, msg);
}

static void _groupActionReceived(Tox*, int group_number, int peer_number, const uint8_t* action, uint16_t length, void* core)
{
    QString act = CString::toString(action, length);
    emit CORE_CAST(core)->groupActionReceived(group_number, peer_number, act);
}

static void _groupTitleChanged(Tox*, int group_number, int peer_number, const uint8_t* title, uint8_t length, void* core)
{
    QString ttl = CString::toString(title, length);
    emit CORE_CAST(core)->groupTitleChanged(group_number, peer_number, ttl);
}

static void _groupNamelistChanged(Tox*, int group_number, int peer_number, uint8_t change, void* core)
{
    emit CORE_CAST(core)->groupNamelistChanged(group_number, peer_number, (TOX_CHAT_CHANGE)change);
}

int ToxCore::addGroupchat()
{
    return tox_add_groupchat(tox);
}

bool ToxCore::delGroupchat(int group_number)
{
    return 0 == tox_del_groupchat(tox, group_number);
}

QString ToxCore::groupPeerName(int group_number, int peer_number) const
{
    QByteArray ret;
    ret.resize(TOX_MAX_NAME_LENGTH);
    int size = tox_group_peername(tox, group_number, peer_number, UI8_CAST(ret.data()));
    if (!CLAMP(size, 0, CLAMP_GTE))
        return QString();
    else
        return QString(ret);
}

QByteArray ToxCore::groupPeerPubkey(int group_number, int peer_number) const
{
    QByteArray ret;
    ret.resize(TOX_PUBLIC_KEY_SIZE);
    if (tox_group_peer_pubkey(tox, group_number, peer_number, UI8_CAST(ret.data())) == -1)
        return QByteArray();
    else
        return ret;
}

bool ToxCore::inviteFriend(int32_t friend_number, int group_number)
{
    return 0 == tox_invite_friend(tox, friend_number, group_number);
}

int ToxCore::joinGroupchat(int32_t friend_number, const QByteArray& data)
{
    return tox_join_groupchat(tox, friend_number, CUI8_CAST(data.data()), data.size());
}

bool ToxCore::groupMessageSend(int group_number, const QString& message)
{
    CString msg(message);
    return 0 == tox_group_message_send(tox, group_number, msg.data(), msg.size());
}

bool ToxCore::groupActionSend(int group_number, const QString& action)
{
    CString act(action);
    return 0 == tox_group_action_send(tox, group_number, act.data(), act.size());
}

bool ToxCore::groupSetTitle(int group_number, const QString& title)
{
    CString ttl(title);
    return 0 == tox_group_set_title(tox, group_number, ttl.data(), ttl.size());
}

QString ToxCore::groupGetTitle(int group_number) // const
{
    QByteArray ret;
    ret.resize(TOX_MAX_NAME_LENGTH);
    int size = tox_group_get_title(tox, group_number, UI8_CAST(ret.data()), TOX_MAX_NAME_LENGTH);
    if (!CLAMP(size, 0, CLAMP_GTE))
        return QString();
    else
        return QString(ret);
}

bool ToxCore::groupPeernumberIsOurs(int group_number, int peer_number) const
{
    return 1 == tox_group_peernumber_is_ours(tox, group_number, peer_number);
}

int ToxCore::groupNumberPeers(int group_number) const
{
    return tox_group_number_peers(tox, group_number);
}

QStringList ToxCore::groupGetNames(int group_number) const
{
    int num = tox_group_number_peers(tox, group_number);
    uint8_t (*names)[TOX_MAX_NAME_LENGTH] = new uint8_t[num][TOX_MAX_NAME_LENGTH];
    // declare names as pointer to array 128 of char (http://cdecl.org)
    uint16_t* lengths = new uint16_t[num];
    if (num != tox_group_get_names(tox, group_number, names, lengths, num))
        return QStringList();
    QStringList ret;
    ret.reserve(num);
    for (int i = 0; i < num; i++)
        ret.append(CString::toString(names[i], lengths[i]));
    delete[] names;
    delete[] lengths;
    return ret;    
}

int ToxCore::groupGetType(int group_number) const
{
    return tox_group_get_type(tox, group_number);
}

bool ToxCore::friendSendLossyPacket(uint32_t friend_number, const QByteArray& data)
{
    if (!CLAMP(data.size(), TOX_MAX_CUSTOM_PACKET_SIZE, CLAMP_LTE))
        return false;
    return QTOX_CALL_CORE(tox_friend_send_lossy_packet, TOX_ERR_FRIEND_CUSTOM_PACKET_OK, tox, friend_number, CUI8_CAST(data.data()), data.size());
}

static void _friendLossyPacketReceived(Tox*, uint32_t friend_number, const uint8_t* data, size_t length, void* core)
{
    QByteArray ret(QBA_CAST(data), length);
    emit CORE_CAST(core)->friendLossyPacketReceived(friend_number, ret);
}

bool ToxCore::friendSendLosslessPacket(uint32_t friend_number, const QByteArray& data)
{
    if (!CLAMP(data.size(), TOX_MAX_CUSTOM_PACKET_SIZE, CLAMP_LTE))
        return false;
    return QTOX_CALL_CORE(tox_friend_send_lossless_packet, TOX_ERR_FRIEND_CUSTOM_PACKET_OK, tox, friend_number, CUI8_CAST(data.data()), data.size());
}

static void _friendLosslessPacketReceived(Tox*, uint32_t friend_number, const uint8_t* data, size_t length, void* core)
{
    QByteArray ret(QBA_CAST(data), length);
    emit CORE_CAST(core)->friendLosslessPacketReceived(friend_number, ret);
}

QByteArray ToxCore::getSelfDhtID() const
{
    QByteArray ret;
    ret.resize(TOX_PUBLIC_KEY_SIZE);
    tox_self_get_dht_id(tox, UI8_CAST(ret.data()));
    return ret;
}

uint16_t ToxCore::getSelfUdpPort() const
{
    // have to check error, but we don't use QVariant either
    TOX_ERR_GET_PORT error;
    uint16_t ret = tox_self_get_udp_port(tox, &error);
    if (error != TOX_ERR_GET_PORT_OK)
    {
        qWarning() << "Error:" << "tox_self_get_udp_port" << "failed with code" << error;
        return 0;
    }
    return ret;
}

uint16_t ToxCore::getSelfTcpPort() const
{
    // have to check error, but we don't use QVariant either
    TOX_ERR_GET_PORT error;
    uint16_t ret = tox_self_get_tcp_port(tox, &error);
    if (error != TOX_ERR_GET_PORT_OK)
    {
        qWarning() << "Error:" << "tox_self_get_tcp_port" << "failed with code" << error;
        return 0;
    }
    return ret;
}

ToxCore::ToxCore(const ToxOptions& options, const QByteArray& data)
{
    const uint8_t* dat;
    size_t size;
    if (data.isEmpty())
    {
        dat = nullptr;
        size = 0;
    }
    else
    {
        dat = CUI8_CAST(data.data());
        size = data.size();
    }
    tox = tox_new(options.tox_options, dat, size, &error);
    if (error == TOX_ERR_NEW_LOAD_BAD_FORMAT) // some data may have loaded
    {
        qWarning() << "Warning: tox_new failed with bad load format, but some data may still be loaded";
    }
    else if (error != TOX_ERR_NEW_OK)
    {
        tox = nullptr;
        qWarning() << "Error:" << "tox_new" << "failed with code" << error;
        return;
    }

    tox_callback_self_connection_status(tox, _selfConnectionStatusChanged, this);
    tox_callback_friend_name(tox, _friendNameChanged, this);
    tox_callback_friend_status_message(tox, _friendStatusMessageChanged, this);
    tox_callback_friend_status(tox, _friendStatusChanged, this);
    tox_callback_friend_connection_status(tox, _friendConnectionStatusChanged, this);
    tox_callback_friend_typing(tox, _friendTypingChanged, this);
    tox_callback_friend_read_receipt(tox, _friendReadReceiptReceived, this);
    tox_callback_friend_request(tox, _friendRequestReceived, this);
    tox_callback_friend_message(tox, _friendMessageReceived, this);
    tox_callback_file_recv_control(tox, _fileControlReceived, this);
    tox_callback_file_chunk_request(tox, _fileChunkRequested, this);
    tox_callback_file_recv(tox, _fileReceiveRequested, this);
    tox_callback_file_recv_chunk(tox, _fileChunkReceived, this);
    tox_callback_group_invite(tox, _groupInviteReceived, this);
    tox_callback_group_message(tox, _groupMessageReceived, this);
    tox_callback_group_action(tox, _groupActionReceived, this);
    tox_callback_group_title(tox, _groupTitleChanged, this);
    tox_callback_group_namelist_change(tox, _groupNamelistChanged, this);
    tox_callback_friend_lossy_packet(tox, _friendLossyPacketReceived, this);
    tox_callback_friend_lossless_packet(tox, _friendLosslessPacketReceived, this);

    emit loaded(getSelfAddress());
    emit usernameSet(getSelfName()); // TODO: remove
    emit statusMessageSet(getSelfStatusMessage()); // TODO: remove
}

TOX_ERR_NEW ToxCore::constructorError() const
{
    return error;
}

ToxCore::~ToxCore()
{
    tox_kill(tox);
}
