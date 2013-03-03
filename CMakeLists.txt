project(akonadi-kde)

add_definitions( -DKDE_DEFAULT_DEBUG_AREA=5250 )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )
if(CMAKE_COMPILE_GCOV)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

if (KDE4_BUILD_TESTS)
   # only with this macro the AKONADI_TESTS_EXPORT macro will do something
   add_definitions(-DCOMPILING_TESTS)
add_subdirectory( tests )
endif()

add_definitions( ${AKONADI_DEFINITIONS} )
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

# Set the resources and agents version
configure_file(kdepimlibs-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/kdepimlibs-version.h)

# Detect the QJson version (0.8.0 is BIC)
if (QJSON_VERSION VERSION_GREATER 0.7.1)
  set(USE_QJSON_0_8 TRUE)
endif()

configure_file(config-akonadi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-akonadi.h)

add_subdirectory( kabc )
add_subdirectory( kcal )
add_subdirectory( kmime )
add_subdirectory( contact )
add_subdirectory( calendar )
add_subdirectory( notes )
add_subdirectory( socialutils )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QT_QTDBUS_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${KDE4_INCLUDE_DIR}
  ${QJSON_INCLUDE_DIR}
  ${AKONADI_INCLUDE_DIR}
  ${AKONADI_INCLUDE_DIR}/akonadi/private
)

# libakonadi-kde

set( akonadikde_LIB_SRC
  entity.cpp # keep it at top to not break enable-final
  actionstatemanager.cpp
  agentactionmanager.cpp
  agentbase.cpp
  agentfactory.cpp
  agentfilterproxymodel.cpp
  agentinstance.cpp
  agentinstancecreatejob.cpp
  agentinstancemodel.cpp
  agentinstancewidget.cpp
  agentmanager.cpp
  agentsearchinterface.cpp
  agenttype.cpp
  agenttypemodel.cpp
  agenttypewidget.cpp
  agenttypedialog.cpp
  asyncselectionhandler.cpp
  attribute.cpp
  attributefactory.cpp
  cachepolicy.cpp
  changemediator_p.cpp
  changerecorder.cpp
  changerecorder_p.cpp
  changenotificationdependenciesfactory.cpp
  collection.cpp
  collectionattributessynchronizationjob.cpp
  collectioncombobox.cpp
  collectioncopyjob.cpp
  collectioncreatejob.cpp
  collectiondeletejob.cpp
  collectionfilterproxymodel.cpp
  collectionfetchjob.cpp
  collectionfetchscope.cpp
  collectionmodel.cpp
  collectionmodel_p.cpp
  collectionmodifyjob.cpp
  collectionmovejob.cpp
  collectionpathresolver.cpp
  collectionquotaattribute.cpp
  collectionrequester.cpp
  collectionrightsattribute.cpp
  collectionselectjob.cpp
  collectionstatistics.cpp
  collectionstatisticsjob.cpp
  collectionstatisticsmodel.cpp
  collectionstatisticsdelegate.cpp
  collectionsync.cpp
  cachepolicypage.cpp
  collectiongeneralpropertiespage.cpp
  collectionpropertiesdialog.cpp
  collectionpropertiespage.cpp
  conflicthandling/conflicthandler.cpp
  conflicthandling/conflictresolvedialog.cpp
  control.cpp
  dbusconnectionpool.cpp
  entitycache.cpp
  entitydisplayattribute.cpp
  entityhiddenattribute.cpp
  entitydeletedattribute.cpp
  entitymimetypefiltermodel.cpp
  entityorderproxymodel.cpp
  entityrightsfiltermodel.cpp
  entitytreemodel.cpp
  entitytreemodel_p.cpp
  entitytreeview.cpp
  erroroverlay.cpp
  etmviewstatesaver.cpp
  exception.cpp
  favoritecollectionsmodel.cpp
  firstrun.cpp
  indexpolicyattribute.cpp
  item.cpp
  itemcreatejob.cpp
  itemcopyjob.cpp
  itemdeletejob.cpp
  itemfetchjob.cpp
  itemfetchscope.cpp
  itemmodel.cpp
  itemmonitor.cpp
  itemmovejob.cpp
  itemsearchjob.cpp
  itemserializer.cpp
  itemserializerplugin.cpp
  itemmodifyjob.cpp
  itemsync.cpp
  invalidatecachejob.cpp
  job.cpp
  kdsignalblocker.cpp
  kjobprivatebase.cpp
  linkjob.cpp
  mimetypechecker.cpp
  monitor.cpp
  monitor_p.cpp
  objectnotificationmessage.cpp
  partfetcher.cpp
  pastehelper.cpp
  persistentsearchattribute.cpp
  preprocessorbase.cpp
  preprocessorbase_p.cpp
  protocolhelper.cpp
  progressspinnerdelegate.cpp
  recursivecollectionfilterproxymodel.cpp
  recursiveitemfetchjob.cpp
  recursivemover.cpp
  resourcebase.cpp
  resourcescheduler.cpp
  resourceselectjob.cpp
  resourcesynchronizationjob.cpp
  searchcreatejob.cpp
  selectionproxymodel.cpp
  selftestdialog.cpp
  session.cpp
  servermanager.cpp
  specialcollectionattribute.cpp
  specialcollections.cpp
  specialcollectionshelperjobs.cpp
  specialcollectionsrequestjob.cpp
  specialcollectionsdiscoveryjob.cpp
  standardactionmanager.cpp
  statisticsproxymodel.cpp
  subscriptiondialog.cpp
  subscriptionjob.cpp
  subscriptionmodel.cpp
  trashjob.cpp
  trashrestorejob.cpp
  trashsettings.cpp
  trashfilterproxymodel.cpp
  transactionjobs.cpp
  transactionsequence.cpp
  transportresourcebase.cpp
  typepluginloader.cpp
  unlinkjob.cpp
# Temporary until ported to Qt-plugin framework
  pluginloader.cpp
  resourcesettings.cpp
  renamefavoritedialog.cpp
  recentcollectionaction.cpp
)

if (KDEPIM_MOBILE_UI)
  set( akonadikde_LIB_SRC ${akonadikde_LIB_SRC}
       collectiondialog_mobile.cpp
  )
else()
  set( akonadikde_LIB_SRC ${akonadikde_LIB_SRC}
       collectiondialog_desktop.cpp
  )
endif()

if (NOT WINCE)
  set(akonadikde_LIB_SRC ${akonadikde_LIB_SRC}
  # pending: fix KIconButton (remove icon selection stuff) and then should be ok
  dragdropmanager.cpp
  # pending: remove xmlguiclient stuff (for context menu) and then should be ok
  collectionview.cpp
  entitylistview.cpp
  entitytreeviewstatesaver.cpp
  itemview.cpp
  )
endif()

# DBus interfaces and adaptors
set(akonadi_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationManager.xml)
set_source_files_properties(${akonadi_xml} PROPERTIES INCLUDE "notificationmessage_p.h")
qt4_add_dbus_interface( akonadikde_LIB_SRC ${akonadi_xml} notificationmanagerinterface )

set(akonadi_xml ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.NotificationSource.xml)
set_source_files_properties(${akonadi_xml} PROPERTIES INCLUDE "notificationmessage_p.h")
qt4_add_dbus_interface( akonadikde_LIB_SRC ${akonadi_xml} notificationsourceinterface )

qt4_add_dbus_interfaces( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.AgentManager.xml )
qt4_add_dbus_interfaces( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Tracer.xml )
qt4_add_dbus_interfaces( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Control.xml )
qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Resource.xml resourcebase.h Akonadi::ResourceBase resourceadaptor Akonadi__ResourceAdaptor )
qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Preprocessor.xml preprocessorbase_p.h Akonadi::PreprocessorBasePrivate preprocessoradaptor Akonadi__PreprocessorAdaptor )
qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Status.xml agentbase.h Akonadi::AgentBase statusadaptor Akonadi__StatusAdaptor )
qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${AKONADI_DBUS_INTERFACES_DIR}/org.freedesktop.Akonadi.Agent.Control.xml agentbase.h Akonadi::AgentBase controladaptor Akonadi__ControlAdaptor)
qt4_add_dbus_adaptor( akonadikde_LIB_SRC interfaces/org.freedesktop.Akonadi.Resource.Transport.xml transportresourcebase_p.h Akonadi::TransportResourceBasePrivate transportadaptor Akonadi__TransportAdaptor )
qt4_add_dbus_adaptor( akonadikde_LIB_SRC interfaces/org.freedesktop.Akonadi.Agent.Search.xml agentsearchinterface_p.h Akonadi::AgentSearchInterfacePrivate searchadaptor Akonadi__SearchAdaptor )


kde4_add_kcfg_files( akonadikde_LIB_SRC resourcebasesettings.kcfgc )
install( FILES resourcebase.kcfg DESTINATION ${KCFG_INSTALL_DIR} )

kde4_add_ui_files( akonadikde_LIB_SRC
  cachepolicypage.ui
  controlprogressindicator.ui
  selftestdialog.ui
  erroroverlay.ui
)

if (KDEPIM_MOBILE_UI)
  kde4_add_ui_files( akonadikde_LIB_SRC
    collectiongeneralpropertiespage_mobile.ui
  )
else()
  kde4_add_ui_files( akonadikde_LIB_SRC
    collectiongeneralpropertiespage.ui
  )
endif()

kde4_add_library( akonadi-kde ${LIBRARY_TYPE} ${akonadikde_LIB_SRC} )

target_link_libraries( akonadi-kde ${QT_QTNETWORK_LIBRARY} ${QT_QTDBUS_LIBRARY} ${QT_QTSQL_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_SOLID_LIBS} ${AKONADI_COMMON_LIBRARIES} )
set( AKONADI_KDE_DEPS ${KDE4_KDEUI_LIBS} ${QT_QTDBUS_LIBRARY} ${QT_QTCORE_LIBRARY} )

if (KDEPIM_MOBILE_UI)
target_link_libraries( akonadi-kde ${QT_QTDECLARATIVE_LIBRARY} )
endif()

target_link_libraries( akonadi-kde LINK_INTERFACE_LIBRARIES ${AKONADI_KDE_DEPS})

set_target_properties( akonadi-kde PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install( TARGETS akonadi-kde EXPORT kdepimlibsLibraryTargets  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( FILES
  akonadi_export.h
  agentactionmanager.h
  agentbase.h
  agentfactory.h
  agentfilterproxymodel.h
  agentinstance.h
  agentinstancecreatejob.h
  agentinstancemodel.h
  agentinstancewidget.h
  agentmanager.h
  agentsearchinterface.h
  agenttype.h
  agenttypemodel.h
  agenttypewidget.h
  agenttypedialog.h
  attribute.h
  attributefactory.h
  cachepolicy.h
  cachepolicypage.h
  changerecorder.h
  collection.h
  collectionattributessynchronizationjob.h
  collectioncombobox.h
  collectioncopyjob.h
  collectioncreatejob.h
  collectiondeletejob.h
  collectiondialog.h
  collectionfilterproxymodel.h
  collectionfetchjob.h
  collectionfetchscope.h
  collectionmodel.h
  collectionmodifyjob.h
  collectionmovejob.h
  collectionpropertiesdialog.h
  collectionpropertiespage.h
  collectionquotaattribute.h
  collectionrequester.h
  collectionstatisticsdelegate.h
  collectionstatisticsmodel.h
  collectionstatistics.h
  collectionstatisticsjob.h
  collectionutils_p.h
  collectionview.h
  conflicthandling/abstractdifferencesreporter.h
  conflicthandling/differencesalgorithminterface.h
  control.h
  dbusconnectionpool.h
  entity.h
  entitydisplayattribute.h
  entityhiddenattribute.h
  entitydeletedattribute.h
  entitylistview.h
  entitymimetypefiltermodel.h
  entityrightsfiltermodel.h
  entityorderproxymodel.h
  entitytreemodel.h
  entitytreeview.h
  entitytreeviewstatesaver.h
  etmviewstatesaver.h
  exception.h
  favoritecollectionsmodel.h
  indexpolicyattribute.h
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
  itemsearchjob.h
  itemserializerplugin.h
  itemsync.h
  itemview.h
  job.h
  linkjob.h
  mimetypechecker.h
  monitor.h
  partfetcher.h
  persistentsearchattribute.h
  preprocessorbase.h
  qtest_akonadi.h
  resourcebase.h
  resourcesynchronizationjob.h
  recursivecollectionfilterproxymodel.h
  recursiveitemfetchjob.h
  searchcreatejob.h
  selectionproxymodel.h
  session.h
  servermanager.h
  specialcollections.h
  specialcollectionsrequestjob.h
  specialcollectionsdiscoveryjob.h
  standardactionmanager.h
  statisticsproxymodel.h
  trashjob.h
  trashrestorejob.h
  trashsettings.h
  trashfilterproxymodel.h
  transactionjobs.h
  transactionsequence.h
  transportresourcebase.h
  unlinkjob.h
  resourcesettings.h
  ${CMAKE_CURRENT_BINARY_DIR}/resourcebasesettings.h
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

if (KDEPIM_MOBILE_UI)
install( FILES
  CollectionDialogMobile.qml
  DESTINATION ${DATA_INSTALL_DIR}/akonadi-kde/qml
)
endif()
