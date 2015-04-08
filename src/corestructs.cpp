#include "src/corestructs.h"
#include <tox/tox.h>
#include <QFile>
#include <QRegularExpression>

#define TOX_ADDR_LENGTH 2*TOX_FRIEND_ADDRESS_SIZE

ToxFile::ToxFile(int FileNum, int FriendId, QByteArray FileName, QString FilePath, FileDirection Direction)
    : fileNum(FileNum), friendId(FriendId), fileName{FileName}, filePath{FilePath}, file{new QFile(filePath)},
    bytesSent{0}, filesize{0}, status{STOPPED}, direction{Direction}, sendTimer{nullptr}
{
}

bool ToxFile::operator==(const ToxFile &other) const
{
    return (fileNum == other.fileNum) && (friendId == other.friendId) && (direction == other.direction);
}

bool ToxFile::operator!=(const ToxFile &other) const
{
    return !(*this == other);
}

void ToxFile::setFilePath(QString path)
{
    filePath=path;
    file->setFileName(path);
}

bool ToxFile::open(bool write)
{
    return write ? file->open(QIODevice::ReadWrite) : file->open(QIODevice::ReadOnly);
}

ToxAddr::ToxAddr(const ToxAddr& other)
{
    publicKey = other.publicKey;
    noSpam = other.noSpam;
    checkSum = other.checkSum;
}

// the following two are convenience functions so I don't have to remember the Qt api
QString ToxAddr::toHexString(const QByteArray& data)
{
    return QString(data.toHex()).toUpper();
}

QByteArray ToxAddr::fromHexString(const QString& hex)
{
    return QByteArray::fromHex(hex.toUtf8());
}

QString ToxAddr::toString() const
{
    return toHexString(publicKey) + toHexString(noSpam) + toHexString(checkSum);
}

ToxAddr::ToxAddr(const QByteArray& addr)
{
    publicKey = addr.left(TOX_ADDR_PUBLIC_KEY_LENGTH);
    noSpam    = addr.mid(TOX_ADDR_PUBLIC_KEY_LENGTH, TOX_ADDR_NO_SPAM_LENGTH);
    checkSum  = addr.mid(TOX_ADDR_PUBLIC_KEY_LENGTH + TOX_ADDR_NO_SPAM_LENGTH, TOX_ADDR_CHECKSUM_LENGTH);
}

ToxAddr::ToxAddr(const QString& addr)
    ToxAddr(fromHexString(addr))
{
}

bool ToxAddr::operator==(const ToxAddr& other) const
{
    return publicKey == other.publicKey;
}

bool ToxAddr::operator!=(const ToxAddr& other) const
{
    return publicKey != other.publicKey;
}

void ToxAddr::clear()
{
    publicKey.clear();
}

bool ToxAddr::isToxAddr(const QString& value)
{
    const QRegularExpression hexRegExp("^[A-Fa-f0-9]+$");
    return value.length() == TOX_ADDR_LENGTH && value.contains(hexRegExp);
}
