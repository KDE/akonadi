# SPDX-FileCopyrightText: 2026 Noham Devillers <nde@enioka.com>
# SPDX-License-Identifier: BSD-2-Clause

set(bindings_library "AkonadiCore")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # clear warnings caused by the generated code
    add_compile_options(
        -Wno-keyword-macro
        -Wno-shadow
    )
endif()

set(wrapped_header ${CMAKE_CURRENT_SOURCE_DIR}/bindings.h)
set(typesystem_file ${CMAKE_CURRENT_SOURCE_DIR}/typesystem_akonadi.xml)

set(dependencies
    KPim6::AkonadiCore
    KF6::ConfigCore
)

set(generated_sources
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadicore_module_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/colorproxymodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_attribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionannotationsattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionidentificationattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectioncolorattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionquotaattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_entityannotationsattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_entitydeletedattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_entitydisplayattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_entityhiddenattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_favoritecollectionattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_indexpolicyattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_persistentsearchattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_specialcollectionattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_tagattribute_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_agentinstancecreatejob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionattributessynchronizationjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectioncopyjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectioncreatejob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectiondeletejob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionfetchjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionmodifyjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionmovejob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionstatisticsjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_itemcopyjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_itemcreatejob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_itemdeletejob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_itemfetchjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_itemmodifyjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_itemmovejob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_itemsearchjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_job_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_recursiveitemfetchjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_resourcesynchronizationjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_searchcreatejob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_specialcollectionsdiscoveryjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_specialcollectionsrequestjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_tagcreatejob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_tagdeletejob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_tagfetchjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_tagmodifyjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_transactionjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_transactionsequence_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_trashjob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_trashrestorejob_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_agentfilterproxymodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_agentinstancefilterproxymodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_agentinstancemodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_agenttypemodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionfilterproxymodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_entitymimetypefiltermodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_entityorderproxymodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_entityrightsfiltermodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_entitytreemodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_favoritecollectionsmodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_recursivecollectionfilterproxymodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_selectionproxymodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_statisticsproxymodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_tagmodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_trashfilterproxymodel_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_abstractdifferencesreporter_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_accountactivitiesabstract_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_agentconfigurationbase_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_agentinstance_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_agentmanager_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_agenttype_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_attributefactory_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_cachepolicy_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_changenotification_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_changerecorder_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collection_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionfetchscope_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionpathresolver_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionstatistics_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_collectionutils_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_control_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_differencesalgorithminterface_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_exception_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_gidextractorinterface_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_item_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_itemfetchscope_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_itemmonitor_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_itemsync_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_mimetypechecker_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_monitor_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_notificationsubscriber_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_partfetcher_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_searchquery_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_searchterm_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_servermanager_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_session_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_specialcollections_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_tag_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_tagcache_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_tagfetchscope_wrapper.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/AkonadiCore/akonadi_trashsettings_wrapper.cpp
)

ecm_generate_python_bindings(
    PACKAGE_NAME ${bindings_library}
    VERSION ${PIM_VERSION}
    WRAPPED_HEADER ${wrapped_header}
    TYPESYSTEM ${typesystem_file}
    EXPORT_TYPESYSTEM
    GENERATED_SOURCES ${generated_sources}
    DEPENDENCIES ${dependencies}
    QT_VERSION ${QT_REQUIRED_VERSION}
    HOMEPAGE_URL "https://invent.kde.org/pim/akonadi"
    ISSUES_URL "https://bugs.kde.org/describecomponents.cgi?product=akonadi"
    AUTHOR "The KDE Community"
    README ../README.md
)

target_link_libraries(
    ${bindings_library}
    PRIVATE
        KPim6AkonadiCore
        KF6::ConfigCore
)

execute_process(
    COMMAND
        ${Python_EXECUTABLE} -Esc
        "import sysconfig; print(sysconfig.get_path('platlib', vars={'platbase': '${CMAKE_INSTALL_PREFIX}', 'base': '${CMAKE_INSTALL_PREFIX}'}))"
    OUTPUT_VARIABLE sysconfig_output
)
string(STRIP ${sysconfig_output} PYTHON_INSTALL_DIR)

install(TARGETS ${bindings_library} LIBRARY DESTINATION "${PYTHON_INSTALL_DIR}")
