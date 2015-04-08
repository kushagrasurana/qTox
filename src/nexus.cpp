#include "nexus.h"
#include "profile.h"
#include "misc/settings.h"
#include "video/camera.h"
#include "widget/gui.h"
#include <QThread>
#include <QDebug>
#include <QImageReader>

#ifdef Q_OS_ANDROID
#include <src/widget/androidgui.h>
#else
#include <src/widget/widget.h>
#endif

static Nexus* nexus{nullptr};

Nexus::Nexus(QObject *parent) :
    QObject(parent)
{
}

Nexus::~Nexus()
{
    delete profile;
#ifdef Q_OS_ANDROID
    delete androidgui;
#else
    delete widget;
#endif
}

void Nexus::start()
{
    if (started)
        return;
    qDebug() << "Nexus: Starting up";

    // Setup the environment
    qRegisterMetaType<Status>("Status");
    qRegisterMetaType<vpx_image>("vpx_image");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<const int16_t*>("const int16_t*");
    qRegisterMetaType<int32_t>("int32_t");
    qRegisterMetaType<int64_t>("int64_t");
    qRegisterMetaType<QPixmap>("QPixmap");
    qRegisterMetaType<ToxFile>("ToxFile");
    qRegisterMetaType<ToxFile::FileDirection>("ToxFile::FileDirection");

    // Create GUI
#ifndef Q_OS_ANDROID
    widget = Nexus::getDesktopGUI();
#endif

    // Create Profile
    QString profilePath = Settings::getInstance().detectProfile();
    profile = new Profile(profilePath);

    // Start GUI
#ifdef Q_OS_ANDROID
    androidgui = new AndroidGUI;
    androidgui->show();
#else
    widget->init();
#endif
    GUI::getInstance();

    // Connections
#ifdef Q_OS_ANDROID
    connect(profile, &Profile::connected, androidgui, &AndroidGUI::onConnected);
    connect(profile, &Profile::disconnected, androidgui, &AndroidGUI::onDisconnected);
    //connect(profile, &Profile::failedToStart, androidgui, &AndroidGUI::onFailedToStartCore);
    //connect(profile, &Profile::badProxy, androidgui, &AndroidGUI::onBadProxyCore);
    connect(profile, &Profile::statusSet, androidgui, &AndroidGUI::onStatusSet);
    connect(profile, &Profile::usernameSet, androidgui, &AndroidGUI::setUsername);
    connect(profile, &Profile::statusMessageSet, androidgui, &AndroidGUI::setStatusMessage);
    connect(profile, &Profile::selfAvatarChanged, androidgui, &AndroidGUI::onSelfAvatarLoaded);

    connect(androidgui, &AndroidGUI::statusSet, profile, &Profile::setStatus);
    //connect(androidgui, &AndroidGUI::friendRequested, profile, &Profile::requestFriendship);
    //connect(androidgui, &AndroidGUI::friendRequestAccepted, profile, &Profile::acceptFriendRequest);
    //connect(androidgui, &AndroidGUI::changeProfile, profile, &Profile::switchConfiguration);
#else
    connect(profile, &Profile::connected,                  widget, &Widget::onConnected);
    connect(profile, &Profile::disconnected,               widget, &Widget::onDisconnected);
    connect(profile, &Profile::failedToStart,              widget, &Widget::onFailedToStartCore);
    connect(profile, &Profile::badProxy,                   widget, &Widget::onBadProxyCore);
    connect(profile, &Profile::statusSet,                  widget, &Widget::onStatusSet);
    connect(profile, &Profile::usernameSet,                widget, &Widget::setUsername);
    connect(profile, &Profile::statusMessageSet,           widget, &Widget::setStatusMessage);
    connect(profile, &Profile::selfAvatarChanged,          widget, &Widget::onSelfAvatarLoaded);
    connect(profile, &Profile::friendAdded,                widget, &Widget::addFriend);
    connect(profile, &Profile::failedToAddFriend,          widget, &Widget::addFriendFailed);
    connect(profile, &Profile::friendUsernameChanged,      widget, &Widget::onFriendUsernameChanged);
    connect(profile, &Profile::friendStatusChanged,        widget, &Widget::onFriendStatusChanged);
    connect(profile, &Profile::friendStatusMessageChanged, widget, &Widget::onFriendStatusMessageChanged);
    connect(profile, &Profile::friendRequestReceived,      widget, &Widget::onFriendRequestReceived);
    connect(profile, &Profile::friendMessageReceived,      widget, &Widget::onFriendMessageReceived);
    connect(profile, &Profile::receiptRecieved,            widget, &Widget::onReceiptRecieved);
    connect(profile, &Profile::groupInviteReceived,        widget, &Widget::onGroupInviteReceived);
    connect(profile, &Profile::groupMessageReceived,       widget, &Widget::onGroupMessageReceived);
    connect(profile, &Profile::groupNamelistChanged,       widget, &Widget::onGroupNamelistChanged);
    connect(profile, &Profile::groupTitleChanged,          widget, &Widget::onGroupTitleChanged);
    connect(profile, &Profile::groupPeerAudioPlaying,      widget, &Widget::onGroupPeerAudioPlaying);
    connect(profile, &Profile::emptyGroupCreated, widget, &Widget::onEmptyGroupCreated);
    connect(profile, &Profile::avInvite, widget, &Widget::playRingtone);
    connect(profile, &Profile::blockingClearContacts, widget, &Widget::clearContactsList, Qt::BlockingQueuedConnection);
    connect(profile, &Profile::friendTypingChanged, widget, &Widget::onFriendTypingChanged);

    connect(profile, SIGNAL(messageSentResult(int,QString,int)), widget, SLOT(onMessageSendResult(int,QString,int)));
    connect(profile, SIGNAL(groupSentResult(int,QString,int)), widget, SLOT(onGroupSendResult(int,QString,int)));

    connect(widget, &Widget::statusSet, profile, &Profile::setStatus);
    connect(widget, &Widget::friendRequested, profile, &Profile::requestFriendship);
    connect(widget, &Widget::friendRequestAccepted, profile, &Profile::acceptFriendRequest);
    connect(widget, &Widget::changeProfile, profile, &Profile::switchConfiguration);
#endif

    // Start Profile
    coreThread->start();

    started = true;
}

Nexus& Nexus::getInstance()
{
    if (!nexus)
        nexus = new Nexus;
    return *nexus;
}

void Nexus::destroyInstance()
{
    delete nexus;
    nexus = nullptr;
}

Profile* Nexus::getProfile()
{
    return getInstance().profile;
}

AndroidGUI* Nexus::getAndroidGUI()
{
    return getInstance().androidgui;
}

Widget* Nexus::getDesktopGUI()
{
    return getInstance().widget;
}

QString Nexus::getSupportedImageFilter()
{
  QString res;
  for (auto type : QImageReader::supportedImageFormats())
    res += QString("*.%1 ").arg(QString(type));
  return tr("Images (%1)", "filetype filter").arg(res.left(res.size()-1));
}

bool Nexus::isFilePathWritable(const QString& filepath)
{
    QFile tmp(filepath);
    bool writable = tmp.open(QIODevice::WriteOnly);
    tmp.remove();
    return writable;
}

QString Nexus::sanitize(QString name)
{
    // these are pretty much Windows banned filename characters
    QList<QChar> banned = {'/', '\\', ':', '<', '>', '"', '|', '?', '*'};
    for (QChar c : banned)
        name.replace(c, '_');
    // also remove leading and trailing periods
    if (name[0] == '.')
        name[0] = '_';
    if (name.endsWith('.'))
        name[name.length()-1] = '_';
    return name;
}
