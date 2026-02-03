set(DBUS_INTERFACE_XMLS
    org.freedesktop.Akonadi.AgentManager.xml
    org.freedesktop.Akonadi.NotificationManager.xml
    org.freedesktop.Akonadi.Preprocessor.xml
    org.freedesktop.Akonadi.Tracer.xml
    org.freedesktop.Akonadi.Agent.Account.xml
    org.freedesktop.Akonadi.Agent.Control.xml
    org.freedesktop.Akonadi.Agent.Search.xml
    org.freedesktop.Akonadi.Agent.Status.xml
    org.freedesktop.Akonadi.Resource.xml
    org.freedesktop.Akonadi.ControlManager.xml
    org.freedesktop.Akonadi.NotificationSource.xml
    org.freedesktop.Akonadi.Server.xml
    org.freedesktop.Akonadi.StorageDebugger.xml
    org.freedesktop.Akonadi.TracerNotification.xml
)

install(FILES ${DBUS_INTERFACE_XMLS} DESTINATION ${AKONADI_DBUS_INTERFACES_INSTALL_DIR})
