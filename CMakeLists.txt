project(akonadi-kde)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )
if(CMAKE_COMPILE_GCOV)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif(CMAKE_COMPILE_GCOV)

if (KDE4_BUILD_TESTS)
   # only with this macro the AKONADI_TESTS_EXPORT macro will do something
   add_definitions(-DCOMPILING_TESTS)
add_subdirectory( tests )
endif (KDE4_BUILD_TESTS)

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

add_subdirectory( kabc )
add_subdirectory( kmime )

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
  entity.cpp # keep it at top to not break enable-final
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
  agenttypedialog.cpp
  attribute.cpp
  attributefactory.cpp
  cachepolicy.cpp
  cachepolicypage.cpp
  changerecorder.cpp
  collection.cpp
  collectioncopyjob.cpp
  collectioncreatejob.cpp
  collectiondeletejob.cpp
  collectiondialog.cpp
  collectionfilterproxymodel.cpp
  collectiongeneralpropertiespage.cpp
  collectionfetchjob.cpp
  collectionmodel.cpp
  collectionmodel_p.cpp
  collectionmodifyjob.cpp
  collectionmovejob.cpp
  collectionpathresolver.cpp
  collectionpropertiesdialog.cpp
  collectionpropertiespage.cpp
  collectionrequester.cpp
  collectionrightsattribute.cpp
  collectionselectjob.cpp
  collectionstatistics.cpp
  collectionstatisticsdelegate.cpp
  collectionstatisticsjob.cpp
  collectionstatisticsmodel.cpp
  collectionsync.cpp
  collectionview.cpp
  control.cpp
  descendantsproxymodel.cpp
  entitydisplayattribute.cpp
  entitytreemodel.cpp
  entitytreemodel_p.cpp
  entityfilterproxymodel.cpp
  erroroverlay.cpp
  exception.cpp
  expungejob.cpp
  favoritecollectionsmodel.cpp
  firstrun.cpp
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
  linkjob.cpp
  mimetypechecker.cpp
  monitor.cpp
  monitor_p.cpp
  pastehelper.cpp
  protocolhelper.cpp
  resourcebase.cpp
  resourcescheduler.cpp
  resourceselectjob.cpp
  searchcreatejob.cpp
  selectionproxymodel.cpp
  selftestdialog.cpp
  session.cpp
  servermanager.cpp
  standardactionmanager.cpp
  statisticsproxymodel.cpp
  statisticstooltipproxymodel.cpp
  subscriptionjob.cpp
  subscriptionchangeproxymodel.cpp
  subscriptiondialog.cpp
  subscriptionmodel.cpp
  transactionjobs.cpp
  transactionsequence.cpp
  unlinkjob.cpp
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
  controlprogressindicator.ui
  selftestdialog.ui
)

kde4_add_library( akonadi-kde SHARED ${akonadikde_LIB_SRC} )

macro_ensure_version( "4.2.0" ${KDE_VERSION} KDE_IS_AT_LEAST_42 )

target_link_libraries( akonadi-kde ${KDE4_SOLID_LIBS} ${QT_QTNETWORK_LIBRARY} ${QT_QTDBUS_LIBRARY} ${QT_QTSQL_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${AKONADI_COMMON_LIBRARIES} )
set( AKONADI_KDE_DEPS ${KDE4_KDEUI_LIBS} ${QT_QTDBUS_LIBRARY} ${QT_QTCORE_LIBRARY} )

if(${KDE_IS_AT_LEAST_42})
target_link_libraries( akonadi-kde LINK_INTERFACE_LIBRARIES ${AKONADI_KDE_DEPS})
else(${KDE_IS_AT_LEAST_42})
target_link_libraries( akonadi-kde ${AKONADI_KDE_DEPS})
endif(${KDE_IS_AT_LEAST_42})


set_target_properties( akonadi-kde PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install( TARGETS akonadi-kde EXPORT kdepimlibsLibraryTargets  ${INSTALL_TARGETS_DEFAULT_ARGS} )

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
  agenttypedialog.h
  attribute.h
  attributefactory.h
  cachepolicy.h
  changerecorder.h
  collection.h
  collectioncopyjob.h
  collectioncreatejob.h
  collectiondeletejob.h
  collectiondialog.h
  collectionfilterproxymodel.h
  collectionfetchjob.h
  collectionmodel.h
  collectionmodifyjob.h
  collectionpropertiesdialog.h
  collectionpropertiespage.h
  collectionrequester.h
  collectionstatisticsdelegate.h
  collectionstatisticsmodel.h
  collectionstatistics.h
  collectionstatisticsjob.h
  collectionview.h
  control.h
  descendantsproxymodel.h
  entity.h
  entitydisplayattribute.h
  entitytreemodel.h
  entityfilterproxymodel.h
  exception.h
  favoritescollectionmodel.h
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
  linkjob.h
  mimetypechecker.h
  monitor.h
  qtest_akonadi.h
  resourcebase.h
  searchcreatejob.h
  selectionproxymodel.h
  session.h
  servermanager.h
  standardactionmanager.h
  statisticsproxymodel.h
  statisticstooltipsproxymodel.h
  transactionjobs.h
  transactionsequence.h
  unlinkjob.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi COMPONENT Devel
)

install( FILES
   collectionpathresolver_p.h
   DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/private COMPONENT Devel
   )

install( FILES
  kcfg2dbus.xsl
  DESTINATION ${DATA_INSTALL_DIR}/akonadi-kde
)
