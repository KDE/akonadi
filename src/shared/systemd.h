#pragma once

#include <QDBusArgument>
#include <QString>
#include <qdbusextratypes.h>

namespace Akonadi::Systemd
{

extern const QString DBusService;
extern const QString DBusPath;

extern const QString AkonadiControlService;
extern const QString AkonadiTarget;

struct UnitFileChanges {
    QString changeType;
    QString symlinkName;
    QString symlinkTarget;
};

QDBusArgument &operator<<(QDBusArgument &arg, const UnitFileChanges &changes);
const QDBusArgument &operator>>(const QDBusArgument &arg, UnitFileChanges &changes);

struct Unit {
    QString unitFile;
    QString description;
    QString loadState;
    QString activeState;
    QString subState;
    QString followedUnit;
    QDBusObjectPath unitPath;
    uint queuedJobId;
    QString jobType;
    QDBusObjectPath jobPath;
};

QDBusArgument &operator<<(QDBusArgument &arg, const Unit &unit);
const QDBusArgument &operator>>(const QDBusArgument &arg, Unit &unit);

void registerDBusTypes();

} // namespace Akonadi::Systemd