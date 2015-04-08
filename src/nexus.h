#ifndef NEXUS_H
#define NEXUS_H

#include <QObject>

class QThread;
class Profile;
class Widget;
class AndroidGUI;

/// This class is in charge of connecting various systems together
/// and forwarding signals appropriately to the right objects
/// It is in charge of starting the GUI and the Core
class Nexus : public QObject
{
    Q_OBJECT
public:
    void start(); ///< Will initialise the systems (GUI, Core, ...)

    static Nexus& getInstance();
    static void destroyInstance();
    static Profile* getProfile(); ///< Will return 0 if not started
    static AndroidGUI* getAndroidGUI(); ///< Will return 0 if not started
    static Widget* getDesktopGUI(); ///< Will return 0 if not started
    static QString getSupportedImageFilter();
    static bool isFilePathWritable(const QString& filepath); // WARNING: Tests by brute force, i.e. removes the file in question
    static QString sanitize(QString filename);

private:
    explicit Nexus(QObject *parent = 0);
    ~Nexus();

private:
    Profile* profile = nullptr; // in the future this may be multiple profiles
    Widget* widget = nullptr;
    AndroidGUI* androidgui = nullptr;
    bool started = false;
};

#endif // NEXUS_H
