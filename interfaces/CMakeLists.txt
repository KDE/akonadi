SET(DBUS_INTERFACE_XMLS
  org.kde.Akonadi.AgentManager.xml
  org.kde.Akonadi.NotificationManager.xml
  org.kde.Akonadi.Tracer.xml
  org.kde.Akonadi.Agent.Control.xml
  org.kde.Akonadi.Agent.Status.xml
  org.kde.Akonadi.Resource.xml
  )

install( FILES ${DBUS_INTERFACE_XMLS}
DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )


