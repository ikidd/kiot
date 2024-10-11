   // SPDX-FileCopyrightText: 1998 Sven Radej <sven@lisa.exp.univie.ac.at>
   //      SPDX-FileCopyrightText: 2006 Dirk Mueller <mueller@kde.org>
   //          SPDX-FileCopyrightText: 2007 Flavio Castelli <flavio.castelli@gmail.com>

#include "core.h"
#include <KIdleTime>
#include <QCoreApplication>
#include <QSocketNotifier>

#include <fcntl.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <sys/ioctl.h>

class CameraWatcher : public QObject
{
    Q_OBJECT
public:
    CameraWatcher(QObject *parent);
    ~CameraWatcher();

private:
    BinarySensor *m_sensor;
    void onInotifyCallback();
    void onInotifyEvent(const inotify_event *event);
    int m_inotifyFd = -1;
    QSocketNotifier *m_notifier;
    QHash<QString, int> m_watchFds;
    int openCount = 0;
};

CameraWatcher::CameraWatcher(QObject *parent)
    : QObject(parent)
{
    m_sensor = new BinarySensor(this);
    m_sensor->setId("camera");
    m_sensor->setName("Camera Active");
    m_sensor->setState(false);

    m_inotifyFd = inotify_init();
    (void)fcntl(m_inotifyFd, F_SETFD, FD_CLOEXEC);

    // 10 cameras should be enough for everyone!
    for (int i = 0 ; i < 10; i++) {
        QString videoDevice = QString("/dev/video%1").arg(i);
        int watchDescriptor = inotify_add_watch(m_inotifyFd, "/dev/video0", IN_OPEN | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE);
        if (watchDescriptor == -1) {
            qWarning() << "Failed to watch" << videoDevice;
            continue;
        }
        m_watchFds[videoDevice] = watchDescriptor;
    }

    m_notifier = new QSocketNotifier(m_inotifyFd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &CameraWatcher::onInotifyCallback);
    m_notifier->setEnabled(true);
}

CameraWatcher::~CameraWatcher()
{
    if (m_inotifyFd != -1) {
        close(m_inotifyFd);
    }
}

void CameraWatcher::onInotifyCallback()
{
    int pending = -1;
    int offsetStartRead = 0; // where we read into buffer
    char buf[8192];
    ioctl(m_inotifyFd, FIONREAD, &pending);

    // copied from KDirWatchPrivate::processInotifyEvents
    while (pending > 0) {
        const int bytesToRead = qMin<int>(pending, sizeof(buf) - offsetStartRead);

        int bytesAvailable = read(m_inotifyFd, &buf[offsetStartRead], bytesToRead);
        pending -= bytesAvailable;
        bytesAvailable += offsetStartRead;
        offsetStartRead = 0;

        int offsetCurrent = 0;
        while (bytesAvailable >= int(sizeof(struct inotify_event))) {
            const struct inotify_event *const event = reinterpret_cast<inotify_event *>(&buf[offsetCurrent]);

            const int eventSize = sizeof(struct inotify_event) + event->len;
            if (bytesAvailable < eventSize) {
                break;
            }

            bytesAvailable -= eventSize;
            offsetCurrent += eventSize;

            onInotifyEvent(event);
        }
        if (bytesAvailable > 0) {
            // copy partial event to beginning of buffer
            memmove(buf, &buf[offsetCurrent], bytesAvailable);
            offsetStartRead = bytesAvailable;
        }
    }
}

void CameraWatcher::onInotifyEvent(const struct inotify_event *event)
{
    if (event->mask & IN_OPEN) {
        openCount++;
    } else if (event->mask & IN_CLOSE_WRITE) {
        openCount--;
    } else if (event->mask & IN_CLOSE_NOWRITE) {
        openCount--;
    }
    Q_ASSERT(openCount >= 0);
    if (openCount == 0 || openCount == 1) {
        m_sensor->setState(openCount > 0);
    }
}

void setupCamera()
{
    new CameraWatcher(qApp);
}

REGISTER_INTEGRATION(setupCamera)
#include "camera.moc"
