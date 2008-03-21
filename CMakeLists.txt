project(akonadi-kde)

# taken from FindQt4.cmake to add additional includes, should probably be merged back
MACRO(QT4_ADD_DBUS_INTERFACE2 _sources _interface _basename _include)
  GET_FILENAME_COMPONENT(_infile ${_interface} ABSOLUTE)
  SET(_header ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.h)
  SET(_impl   ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.cpp)
  SET(_moc    ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.moc)
  SET(_params -m -i ${_include} -p)

  ADD_CUSTOM_COMMAND(OUTPUT ${_impl} ${_header}
      COMMAND ${QT_DBUSXML2CPP_EXECUTABLE} ${_params} ${_basename} ${_infile}
      DEPENDS ${_infile})

  SET_SOURCE_FILES_PROPERTIES(${_impl} PROPERTIES SKIP_AUTOMOC TRUE)

  QT4_GENERATE_MOC(${_header} ${_moc})

  SET(${_sources} ${${_sources}} ${_impl} ${_header} ${_moc})
  MACRO_ADD_FILE_DEPENDENCIES(${_impl} ${_moc})
ENDMACRO(QT4_ADD_DBUS_INTERFACE2)

configure_file(akonadi-prefix.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/akonadi-prefix.h)

add_subdirectory( tests )

include_directories(
  ${CMAKE_SOURCE_DIR}/
  ${CMAKE_SOURCE_DIR}/akonadi
  ${CMAKE_BINARY_DIR}/akonadi
  ${QT_QTDBUS_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

# libakonadiprotocolinternals
set( akonadiprotocolinternals_srcs
  imapparser.cpp
  imapset.cpp
  notificationmessage.cpp
  xdgbasedirs.cpp
)

kde4_add_library( akonadiprotocolinternals SHARED ${akonadiprotocolinternals_srcs} )

target_link_libraries( akonadiprotocolinternals ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY})
set_target_properties( akonadiprotocolinternals PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install( TARGETS akonadiprotocolinternals DESTINATION ${LIB_INSTALL_DIR} )

# libakonadi-kde

set( akonadikde_LIB_SRC
  agentbase.cpp
  agentfilterproxymodel.cpp
  agentinstancecreatejob.cpp
  agentinstancemodel.cpp
  agentinstanceview.cpp
  agentmanager.cpp
  agenttypemodel.cpp
  agenttypeview.cpp
  cachepolicy.cpp
  cachepolicypage.cpp
  changerecorder.cpp
  collection.cpp
  collectionattribute.cpp
  collectionattributefactory.cpp
  collectioncombobox.cpp
  collectioncopyjob.cpp
  collectioncreatejob.cpp
  collectiondeletejob.cpp
  collectionfilterproxymodel.cpp
  collectiongeneralpropertiespage.cpp
  collectionlistjob.cpp
  collectionmodel.cpp
  collectionmodifyjob.cpp
  collectionpathresolver.cpp
  collectionpropertiesdialog.cpp
  collectionpropertiespage.cpp
  collectionrightsattribute.cpp
  collectionselectjob.cpp
  collectionstatus.cpp
  collectionstatusjob.cpp
  collectionsync.cpp
  collectionview.cpp
  control.cpp
  datareference.cpp
  expungejob.cpp
  flatcollectionproxymodel.cpp
  item.cpp
  itemappendjob.cpp
  itembrowser.cpp
  itemcopyjob.cpp
  itemdeletejob.cpp
  itemdetailsview.cpp
  itemfetchjob.cpp
  itemmodel.cpp
  itemserializer.cpp
  itemserializerplugin.cpp
  itemstorejob.cpp
  itemsync.cpp
  itemview.cpp
  job.cpp
  messagecollectionmodel.cpp
  monitor.cpp
  monitor_p.cpp
  pastehelper.cpp
  profilemanager.cpp
  profilemodel.cpp
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
# Temporary until ported to Qt-plugin framework
  pluginloaderbase.cpp
)

# DBus interfaces and adaptors
qt4_generate_dbus_interface( resource.h org.kde.Akonadi.Resource.xml )
qt4_generate_dbus_interface( agent.h org.kde.Akonadi.Agent.xml )
qt4_add_dbus_interface2( akonadikde_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/interfaces/org.kde.Akonadi.NotificationManager.xml notificationmanagerinterface notificationmessage.h )
qt4_add_dbus_interfaces( akonadikde_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/interfaces/org.kde.Akonadi.AgentManager.xml )
qt4_add_dbus_interfaces( akonadikde_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/interfaces/org.kde.Akonadi.ProfileManager.xml )
qt4_add_dbus_interfaces( akonadikde_LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/interfaces/org.kde.Akonadi.Tracer.xml )
qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Resource.xml resourcebase.h Akonadi::ResourceBase )
qt4_add_dbus_adaptor( akonadikde_LIB_SRC ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Agent.xml agentbase.h Akonadi::AgentBase )

kde4_add_ui_files( akonadikde_LIB_SRC
  cachepolicypage.ui
  collectiongeneralpropertiespage.ui
  subscriptiondialog.ui
)

kde4_add_library( akonadi-kde SHARED ${akonadikde_LIB_SRC} )

target_link_libraries( akonadi-kde ${QT_QTNETWORK_LIBRARY} ${QT_QTDBUS_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDE4_KDE3SUPPORT_LIBS} akonadiprotocolinternals )
set_target_properties( akonadi-kde PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install( TARGETS akonadi-kde  DESTINATION ${LIB_INSTALL_DIR} )

########### install files ###############

install( FILES
  akonadi_export.h
  agentbase.h
  agentfilterproxymodel.h
  agentinstancecreatejob.h
  agentinstancemodel.h
  agentmanager.h
  agenttypemodel.h
  cachepolicy.h
  collection.h
  collectionattribute.h
  collectionattributefactory.h
  collectioncopyjob.h
  collectioncreatejob.h
  collectiondeletejob.h
  collectionfilterproxymodel.h
  collectionlistjob.h
  collectionmodel.h
  collectionmodifyjob.h
  collectionpathresolver.h
  collectionpropertiesdialog.h
  collectionpropertiespage.h
  collectionselectjob.h
  collectionstatus.h
  collectionstatusjob.h
  collectionview.h
  control.h
  datareference.h
  expungejob.h
  item.h
  itemappendjob.h
  itembrowser.h
  itemcopyjob.h
  itemdeletejob.h
  itemdetailsview.h
  itemfetchjob.h
  itemmodel.h
  itempayloadinternals_p.h
  itemserializerplugin.h
  itemstorejob.h
  itemsync.h
  itemview.h
  job.h
  messagecollectionmodel.h
  monitor.h
  profilemanager.h
  profilemodel.h
  resource.h
  resourcebase.h
  session.h
  standardactionmanager.h
  transactionjobs.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi
)

install( FILES
  interfaces/org.kde.Akonadi.AgentManager.xml
  interfaces/org.kde.Akonadi.NotificationManager.xml
  interfaces/org.kde.Akonadi.ProfileManager.xml
  interfaces/org.kde.Akonadi.Tracer.xml
  ${CMAKE_CURRENT_BINARY_DIR}/org.kde.Akonadi.Resource.xml
DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
