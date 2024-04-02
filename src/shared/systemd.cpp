#include "systemd.h"
#include <QDBusMetaType>
#include <QMetaType>
#include <qdbusargument.h>
#include <qdbusmetatype.h>

Q_DECLARE_METATYPE(Akonadi::Systemd::UnitFileChanges)
Q_DECLARE_METATYPE(QList<Akonadi::Systemd::UnitFileChanges>)
Q_DECLARE_METATYPE(Akonadi::Systemd::Unit)
Q_DECLARE_METATYPE(QList<Akonadi::Systemd::Unit>)

namespace Akonadi::Systemd
{

const QString DBusService = QStringLiteral("org.freedesktop.systemd1");
const QString DBusPath = QStringLiteral("/org/freedesktop/systemd1");

const QString AkonadiControlService = QStringLiteral("akonadi_control.service");
const QString AkonadiTarget = QStringLiteral("akonadi.target");

const QDBusArgument &operator>>(const QDBusArgument &arg, UnitFileChanges &changes)
{
    arg.beginStructure();
    arg >> changes.changeType >> changes.symlinkName >> changes.symlinkTarget;
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const UnitFileChanges &changes)
{
    arg.beginStructure();
    arg << changes.changeType << changes.symlinkName << changes.symlinkTarget;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Unit &unit)
{
    arg.beginStructure();
    arg >> unit.unitFile >> unit.description >> unit.loadState >> unit.activeState >> unit.subState >> unit.followedUnit >> unit.unitPath >> unit.queuedJobId
        >> unit.jobType >> unit.jobPath;
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Unit &unit)
{
    arg.beginStructure();
    arg << unit.unitFile << unit.description << unit.loadState << unit.activeState << unit.subState << unit.followedUnit << unit.unitPath << unit.queuedJobId
        << unit.jobType << unit.jobPath;
    arg.endStructure();
    return arg;
}

void registerDBusTypes()
{
    static bool called = false;
    if (called) {
        return;
    }
    called = true;

    qDBusRegisterMetaType<UnitFileChanges>();
    qDBusRegisterMetaType<QList<UnitFileChanges>>();
    qDBusRegisterMetaType<Unit>();
    qDBusRegisterMetaType<QList<Unit>>();
}

} // namespace Akonadi::Systemd
