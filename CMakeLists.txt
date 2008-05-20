project(akonadi-kde)

if (KDE4_BUILD_TESTS)
   # only with this macro the AKONADI_TESTS_EXPORT macro will do something
   add_definitions(-DCOMPILING_TESTS)
endif (KDE4_BUILD_TESTS)

add_subdirectory( kmime )
add_subdirectory( tests )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QT_QTDBUS_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${KDE4_INCLUDE_DIR}
  ${AKONADI_INCLUDE_DIR}
  ${AKONADI_INCLUDE_DIR}/akonadi/private
)

# libakonadi-kde

set( akonadikde_LIB_SRC
  agentbase.cpp
  agentfilterproxymodel.cpp
  agentinstance.cpp
  agentinstancecreatejob.cpp
  agentinstancemodel.cpp
  agentinstancewidget.cpp
  agentmanager.cpp
  agenttype.cpp
  agenttypemodel.cpp
  agenttypewidget.cpp
  attribute.cpp
  attributefactory.cpp
  cachepolicy.cpp
  cachepolicypage.cpp
  changerecorder.cpp
  collection.cpp
  collectioncopyjob.cpp
  collectioncreatejob.cpp
  collectiondeletejob.cpp
  collectionfilterproxymodel.cpp
  collectiongeneralpropertiespage.cpp
  collectionfetchjob.cpp
  collectionmodel.cpp
  collectionmodel_p.cpp
  collectionmodifyjob.cpp
  collectionpathresolver.cpp
  collectionpropertiesdialog.cpp
  collectionpropertiespage.cpp
  collectionrightsattribute.cpp
  collectionselectjob.cpp
  collectionstatistics.cpp
  collectionstatisticsdelegate.cpp
  collectionstatisticsjob.cpp
  collectionstatisticsmodel.cpp
  collectionsync.cpp
  collectionview.cpp
  control.cpp
  entity.cpp
  expungejob.cpp
  flatcollectionproxymodel.cpp
  item.cpp
  itemcreatejob.cpp
  itemcopyjob.cpp
  itemdeletejob.cpp
  itemfetchjob.cpp
  itemfetchscope.cpp
  itemmodel.cpp
  itemmonitor.cpp
  itemmovejob.cpp
  itemserializer.cpp
  itemserializerplugin.cpp
  itemmodifyjob.cpp
  itemsync.cpp
  itemview.cpp
  job.cpp
  monitor.cpp
  monitor_p.cpp
  pastehelper.cpp
  protocolhelper.cpp
  resourcebase.cpp
  resourcescheduler.cpp
  searchcreatejob.cpp
  session.cpp
  standardactionmanager.cpp
  subscriptionjob.cpp
  subscriptionchangeproxymodel.cpp
  subscriptiondialog.cpp
  subscriptionmodel.cpp
  transactionjobs.cpp
  transactionsequence.cpp
# Temporary until ported to Qt-plugin framework
  pluginloader.cpp
)

# DBus interfaces and adaptors
set(akonadi_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationManager.xml)
set_source_files_properties(${akonadi_xml} PROPERTIES INCLUDE "notificationmessage_p.h")
qt4_add_dbus_interface( akonadikde_LIB_SRC ${akonadi_xml} notificationmanagerinterface )
qt4_add_dbus_interfaces( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.AgentManager.xml )
qt4_add_dbus_interfaces( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Tracer.xml )
qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Resource.xml resourcebase.h Akonadi::ResourceBase )
qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Status.xml agentbase.h Akonadi::AgentBase )
qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Control.xml agentbase.h Akonadi::AgentBase )

kde4_add_ui_files( akonadikde_LIB_SRC
  cachepolicypage.ui
  collectiongeneralpropertiespage.ui
  subscriptiondialog.ui
)

kde4_add_library( akonadi-kde SHARED ${akonadikde_LIB_SRC} )

target_link_libraries( akonadi-kde ${QT_QTNETWORK_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDE4_KDE3SUPPORT_LIBS} ${AKONADI_COMMON_LIBRARIES} )
set_target_properties( akonadi-kde PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install( TARGETS akonadi-kde  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES
  akonadi_export.h
  agentbase.h
  agentfilterproxymodel.h
  agentinstance.h
  agentinstancecreatejob.h
  agentinstancemodel.h
  agentinstancewidget.h
  agentmanager.h
  agenttype.h
  agenttypemodel.h
  agenttypewidget.h
  attribute.h
  attributefactory.h
  cachepolicy.h
  changerecorder.h
  collection.h
  collectioncopyjob.h
  collectioncreatejob.h
  collectiondeletejob.h
  collectionfilterproxymodel.h
  collectionfetchjob.h
  collectionmodel.h
  collectionmodifyjob.h
  collectionpropertiesdialog.h
  collectionpropertiespage.h
  collectionstatisticsdelegate.h
  collectionstatisticsmodel.h
  collectionstatistics.h
  collectionstatisticsjob.h
  collectionview.h
  control.h
  entity.h
  item.h
  itemcreatejob.h
  itemcopyjob.h
  itemdeletejob.h
  itemfetchjob.h
  itemfetchscope.h
  itemmodel.h
  itemmodifyjob.h
  itemmonitor.h
  itemmovejob.h
  itempayloadinternals_p.h
  itemserializerplugin.h
  itemsync.h
  itemview.h
  job.h
  monitor.h
  resourcebase.h
  session.h
  standardactionmanager.h
  transactionjobs.h
  transactionsequence.h
  # temporary test
  searchcreatejob.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi
)

install( FILES
   collectionpathresolver_p.h
   DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/private
   )

install( FILES
  kcfg2dbus.xsl
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/apps/akonadi-kde
)
