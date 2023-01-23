# akonadi test fake library
set(akonaditestfake_xml ${Akonadi_SOURCE_DIR}/src/interfaces/org.freedesktop.Akonadi.NotificationSource.xml)
set_source_files_properties(${akonaditestfake_xml} PROPERTIES INCLUDE "protocol_p.h")
qt_add_dbus_interface( akonaditestfake_srcs ${akonaditestfake_xml} notificationsourceinterface )

add_library(akonaditestfake SHARED
  ${akonaditestfake_srcs}
  fakeakonadiservercommand.cpp
  fakesession.cpp
  fakemonitor.cpp
  fakeserverdata.cpp
  modelspy.cpp
  fakeentitycache.cpp
  inspectablemonitor.cpp
  inspectablechangerecorder.cpp
  fakeakonadiservercommand.h
  fakesession.h
  fakemonitor.h
  fakeserverdata.h
  modelspy.h
  fakeentitycache.h
  inspectablemonitor.h
  inspectablechangerecorder.h
)

generate_export_header(akonaditestfake BASE_NAME akonaditestfake)

target_link_libraries(akonaditestfake
        Qt::DBus
        KF5::AkonadiCore
        Qt::Test
        Qt::Widgets
        Qt::Network
        KF5::AkonadiPrivate
        akonadi_shared
)

add_executable(akonadi-firstrun
    ../../src/core/firstrun.cpp
    firstrunner.cpp
    ${CMAKE_BINARY_DIR}/src/core/akonadicore_debug.cpp
)
target_link_libraries( akonadi-firstrun  Qt::Test Qt::Core KF5::AkonadiCore KF5::AkonadiPrivate KF${KF_MAJOR_VERSION}::ConfigCore Qt::Widgets)

add_akonadi_test(itemhydratest.cpp itemhydratest.h)
add_akonadi_test(itemtest.cpp itemtest.h)
add_akonadi_test(itemserializertest.cpp itemserializertest.h)
add_akonadi_test(mimetypecheckertest.cpp mimetypecheckertest.h)
add_akonadi_test(protocolhelpertest.cpp)
add_akonadi_test(entitytreemodeltest.cpp)
add_akonadi_test(monitornotificationtest.cpp)
add_akonadi_test(collectionutilstest.cpp)
add_akonadi_test(collectioncolorattributetest.cpp)
add_akonadi_test(entitydisplayattributetest.cpp)
add_akonadi_test(proxymodelstest.cpp)
add_akonadi_test_widgets(actionstatemanagertest.cpp)
add_akonadi_test_widgets(conflictresolvedialogtest.cpp conflictresolvedialogtest.h)
add_akonadi_test(tagmodeltest.cpp)
add_akonadi_test(statisticsproxymodeltest.cpp)

add_akonadi_test(sharedvaluepooltest.cpp)
add_akonadi_test(jobtest.cpp)
add_akonadi_test(tagtest_simple.cpp)
add_akonadi_test(cachepolicytest.cpp cachepolicytest.h)

# PORT FROM QJSON add_akonadi_test(searchquerytest.cpp)

# qtestlib tests that need non-exported stuff from 
#add_executable( resourceschedulertest resourceschedulertest.cpp resourceschedulertest.h ../src/agentbase/resourcescheduler.cpp )
#add_test( resourceschedulertest resourceschedulertest )
#ecm_mark_as_test(akonadi-resourceschedulertest)
#target_link_libraries(resourceschedulertest  Qt::Test KF5::AkonadiAgentBase)


# testrunner tests
add_akonadi_isolated_test(SOURCE testenvironmenttest.cpp)
add_akonadi_isolated_test(SOURCE autoincrementtest.cpp autoincrementtest.h)
add_akonadi_isolated_test(SOURCE attributefactorytest.cpp attributefactorytest.h)
add_akonadi_isolated_test(SOURCE collectionpathresolvertest.cpp collectionpathresolvertest.h)
add_akonadi_isolated_test(SOURCE collectionattributetest.cpp collectionattributetest.h)
add_akonadi_isolated_test(SOURCE itemfetchtest.cpp itemfetchtest.h)
add_akonadi_isolated_test(SOURCE itemappendtest.cpp itemappendtest.h)
add_akonadi_isolated_test(SOURCE itemstoretest.cpp itemstoretest.h)
add_akonadi_isolated_test(SOURCE itemdeletetest.cpp)
add_akonadi_isolated_test(SOURCE entitycachetest.cpp)
add_akonadi_isolated_test(SOURCE monitortest.cpp monitortest.h)
#add_akonadi_isolated_test_advanced(monitorfiltertest.cpp "" "KF5::AkonadiPrivate")
# FIXME: this is constantly failling due to broken search: re-enable once the new search code is merged
#add_akonadi_isolated_test(SOURCE searchjobtest.cpp searchjobtest.h)
add_akonadi_isolated_test(SOURCE changerecordertest.cpp)
add_akonadi_isolated_test(SOURCE resourcetest.cpp)
add_akonadi_isolated_test(SOURCE subscriptiontest.cpp)
add_akonadi_isolated_test(SOURCE transactiontest.cpp transactiontest.h)
add_akonadi_isolated_test(SOURCE itemcopytest.cpp)
add_akonadi_isolated_test(SOURCE itemmovetest.cpp)
add_akonadi_isolated_test(SOURCE invalidatecachejobtest.cpp)
add_akonadi_isolated_test(SOURCE collectioncreatetest.cpp)
add_akonadi_isolated_test(SOURCE collectioncopytest.cpp)
add_akonadi_isolated_test(SOURCE collectionmovetest.cpp)
add_akonadi_isolated_test(
    SOURCE  collectionsynctest.cpp
    ADDITIONAL_SOURCES ${CMAKE_BINARY_DIR}/src/core/akonadicore_debug.cpp
    LINK_LIBRARIES KF${KF_MAJOR_VERSION}::I18n
)
add_akonadi_isolated_test(SOURCE itemsynctest.cpp)
add_akonadi_isolated_test(SOURCE linktest.cpp)
add_akonadi_isolated_test(SOURCE cachetest.cpp)
add_akonadi_isolated_test(SOURCE collectionjobtest.cpp collectionjobtest.h)
add_akonadi_isolated_test(SOURCE collectionmodifytest.cpp)

# FIXME: This is very unstable on Jenkins
#add_akonadi_isolated_test(servermanagertest.cpp)

# Having a benchmark is cool if you have any reference to compare against, but this
# benchmark takes over 40 seconds and does not have any real value to us atm. Major
# performance regressions would be spotted by devs anyway, so disabling for now.
#add_akonadi_isolated_test(itembenchmark.cpp)
#add_akonadi_isolated_test(collectioncreator.cpp)

add_akonadi_isolated_test(SOURCE gidtest.cpp gidtest.h)
add_akonadi_isolated_test(SOURCE lazypopulationtest.cpp)
add_akonadi_isolated_test(SOURCE favoriteproxytest.cpp LINK_LIBRARIES KF${KF_MAJOR_VERSION}::ConfigCore)
# FIXME: this is constantly failing due to broken search: re-enable once the new search code is merged
#add_akonadi_isolated_test(
#    SOURCE itemsearchjobtest.cpp searchjobtest.h
#    ADDITIONAL_SOURCES testsearchplugin/testsearchplugin.cpp)
add_akonadi_isolated_test(SOURCE tagtest.cpp ADDITIONAL_SOURCES ${CMAKE_BINARY_DIR}/src/core/akonadicore_debug.cpp)
add_akonadi_isolated_test(SOURCE tagsynctest.cpp)
add_akonadi_isolated_test(SOURCE relationtest.cpp)
add_akonadi_isolated_test(SOURCE etmpopulationtest.cpp)
