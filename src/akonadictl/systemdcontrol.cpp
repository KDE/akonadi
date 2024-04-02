#include "systemdcontrol.h"
#include "shared/systemd.h"
#include "systemdmanagerinterface.h"

#include <QDBusConnection>

#include <algorithm>
#include <iostream>

using namespace Akonadi;

SystemDControl::SystemDControl()
{
    Systemd::registerDBusTypes();
}

SystemDControl::~SystemDControl()
{
}

Status SystemDControl::status() const
{
    org::freedesktop::systemd1::Manager manager(Systemd::DBusService, Systemd::DBusPath, QDBusConnection::sessionBus());
    if (!manager.isValid()) {
        return Status::Unknown;
    }

    const auto reply = manager.ListUnitsByNames({Systemd::AkonadiTarget});
    if (reply.isError()) {
        std::cerr << "Failed to query state of the" << qUtf8Printable(Systemd::AkonadiTarget) << " Systemd unit: " << reply.error().message().toStdString()
                  << "\n";
        return Status::Unknown;
    }

    const auto units = reply.value();
    auto control = std::find_if(units.begin(), units.end(), [](const Systemd::Unit &unit) {
        return unit.unitFile == Systemd::AkonadiTarget;
    });
    if (control == units.end()) {
        std::cerr << "No unit named " << qUtf8Printable(Systemd::AkonadiTarget) << " registered to Systemd, please check your Akonadi installation.\n";
        return Status::Unknown;
    }

    if (control->activeState == u"active") {
        return Status::Running;
    } else if (control->activeState == u"activating") {
        return Status::Starting;
    } else if (control->activeState == u"deactivating") {
        return Status::Stopping;
    } else if (control->activeState == u"inactive") {
        return Status::Stopped;
    } else if (control->activeState == u"failed") {
        return Status::Error;
    }

    return Status::Unknown;
}

bool SystemDControl::start()
{
    org::freedesktop::systemd1::Manager manager(Systemd::DBusService, Systemd::DBusPath, QDBusConnection::sessionBus());
    if (!manager.isValid()) {
        return false;
    }

    manager.StartUnit(Systemd::AkonadiTarget, QStringLiteral("replace"));

    return true;
}

bool SystemDControl::stop()
{
    org::freedesktop::systemd1::Manager manager(Systemd::DBusService, Systemd::DBusPath, QDBusConnection::sessionBus());
    if (!manager.isValid()) {
        return false;
    }

    manager.StopUnit(Systemd::AkonadiTarget, QStringLiteral("replace"));

    return true;
}
