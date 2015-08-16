/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    Inspired by kdelibs/kdecore/io/kdebug.h
    Copyright (C) 1997 Matthias Kalle Dalheimer (kalle@kde.org)
                  2002 Holger Freyther (freyther@kde.org)

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "akdebug.h"
#include "akcrash.h"
#include "akstandarddirs.h"

#include <private/xdgbasedirs_p.h>
using Akonadi::XdgBaseDirs;

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>

#include <cassert>

class FileDebugStream : public QIODevice
{
public:
    FileDebugStream()
        : mType(QtCriticalMsg)
    {
        open(WriteOnly);
    }

    bool isSequential() const
    {
        return true;
    }
    qint64 readData(char *, qint64)
    {
        return 0;
    }
    qint64 readLineData(char *, qint64)
    {
        return 0;
    }
    qint64 writeData(const char *data, qint64 len)
    {
        QByteArray buf = QByteArray::fromRawData(data, len);

        if (!mFileName.isEmpty()) {
            QFile outputFile(mFileName);
            outputFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Unbuffered);
            outputFile.write(data, len);
            outputFile.putChar('\n');
            outputFile.close();
        }

        qt_message_output(mType,
                          QMessageLogContext(),
                          QString::fromLatin1(buf.trimmed()));
        return len;
    }

    void setFileName(const QString &fileName)
    {
        mFileName = fileName;
    }

    void setType(QtMsgType type)
    {
        mType = type;
    }
private:
    QString mFileName;
    QtMsgType mType;
};

class DebugPrivate
{
public:
    DebugPrivate()
        : fileStream(new FileDebugStream())
    {
    }

    ~DebugPrivate()
    {
        delete fileStream;
    }

    QString errorLogFileName() const
    {
        return AkStandardDirs::saveDir("data")
               + QDir::separator()
               + name
               + QLatin1String(".error");
    }

    QDebug stream(QtMsgType type)
    {
        QMutexLocker locker(&mutex);
#ifndef QT_NO_DEBUG_OUTPUT
        if (type == QtDebugMsg) {
            return qDebug();
        }
#endif
        fileStream->setType(type);
        return QDebug(fileStream);
    }

    void setName(const QString &appName)
    {
        // Keep only the executable name, e.g. akonadi_control
        name = appName.mid(appName.lastIndexOf(QLatin1Char('/')) + 1);
        fileStream->setFileName(errorLogFileName());
    }

    QMutex mutex;
    FileDebugStream *fileStream;
    QString name;
};

Q_GLOBAL_STATIC(DebugPrivate, sInstance)

QDebug akFatal()
{
    return sInstance()->stream(QtFatalMsg);
}

QDebug akError()
{
    return sInstance()->stream(QtCriticalMsg);
}

#ifndef QT_NO_DEBUG_OUTPUT
QDebug akDebug()
{
    return sInstance()->stream(QtDebugMsg);
}
#endif

void akInit(const QString &appName)
{
    AkonadiCrash::init();
    sInstance()->setName(appName);

    QFileInfo infoOld(sInstance()->errorLogFileName() + QLatin1String(".old"));
    if (infoOld.exists()) {
        QFile fileOld(infoOld.absoluteFilePath());
        const bool success = fileOld.remove();
        if (!success) {
            qFatal("Cannot remove old log file - running on a readonly filesystem maybe?");
        }
    }
    QFileInfo info(sInstance()->errorLogFileName());
    if (info.exists()) {
        QFile file(info.absoluteFilePath());
        const bool success = file.rename(sInstance()->errorLogFileName() + QLatin1String(".old"));
        if (!success) {
            qFatal("Cannot rename log file - running on a readonly filesystem maybe?");
        }
    }
}

QString getEnv(const char *name, const QString &defaultValue)
{
    const QString v = QString::fromLocal8Bit(qgetenv(name));
    return !v.isEmpty() ? v : defaultValue;
}
