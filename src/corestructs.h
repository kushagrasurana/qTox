#ifndef CORESTRUCTS_H
#define CORESTRUCTS_H

// Some headers use Core structs but don't need to include all of core.h
// They should include this file directly instead to reduce compilation times

#include <QString>
class QFile;
class QTimer;

enum class Status : int {Online = 0, Away, Busy, Offline}; // TODO: replace with tox.h enums

// TODO: replace magic numbers with compile-time checkable constants (without including tox.h)
// assertion in corestructs.cpp?
#define TOX_ADDR_PUBLIC_KEY_LENGTH 64
#define TOX_ADDR_NO_SPAM_LENGTH    8
#define TOX_ADDR_CHECKSUM_LENGTH   4

struct ToxAddr // TODO: core no longer uses the term "id". There is your public key and address (which is pk+nospam+checksum)
{
    ToxAddr()=default;
    explicit ToxAddr(const ToxAddr& other);
    explicit ToxAddr(const QString& addr);
    explicit ToxAddr(const QByteArray& addr);

    QByteArray publicKey;
    QByteArray noSpam;
    QByteArray checkSum;

    QString toString() const;
    static bool isToxAddr(const QString& addr);

    bool operator==(const ToxAddr& other) const;
    bool operator!=(const ToxAddr& other) const;
    void clear();

    static QString toHexString(const QByteArray& data);
    static QByteArray fromHexString(const QString& hex);
};

struct DhtServer
{
    QString name;
    QString userId;
    QString address;
    int port;
};

struct ToxFile
{
    enum FileStatus
    {
        STOPPED,
        PAUSED,
        TRANSMITTING,
        BROKEN
    };

    enum FileDirection : bool
    {
        SENDING,
        RECEIVING
    };

    ToxFile()=default;
    ToxFile(int FileNum, int FriendId, QByteArray FileName, QString FilePath, FileDirection Direction);
    ~ToxFile(){}

    bool operator==(const ToxFile& other) const;
    bool operator!=(const ToxFile& other) const;

    void setFilePath(QString path);
    bool open(bool write);

    int fileNum;
    int friendId;
    QByteArray fileName;
    QString filePath;
    QFile* file;
    qint64 bytesSent;
    qint64 filesize;
    FileStatus status;
    FileDirection direction;
    QTimer* sendTimer;
};

#endif // CORESTRUCTS_H
