SET(DBUS_INTERFACE_XMLS
  org.freedesktop.Akonadi.AgentManager.xml
  org.freedesktop.Akonadi.NotificationManager.xml
  org.freedesktop.Akonadi.Tracer.xml
  org.freedesktop.Akonadi.Agent.Control.xml
  org.freedesktop.Akonadi.Agent.Status.xml
  org.freedesktop.Akonadi.Resource.xml
  )

install( FILES ${DBUS_INTERFACE_XMLS}
DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )


