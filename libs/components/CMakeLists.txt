add_subdirectory( tests )

include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${QT_QTDBUS_INCLUDE_DIR}
)

########### next target ###############

set( akonadi_components_LIB_SRC
	agentinstanceview.cpp
	agenttypeview.cpp
	profileview.cpp
)

qt4_add_resources( akonadi_components_LIB_SRC agentinstanceview.qrc )

kde4_automoc(${akonadi_components_LIB_SRC})
kde4_add_library( akonadicomponents SHARED ${akonadi_components_LIB_SRC} )
target_link_libraries( akonadicomponents akonadi )
set_target_properties( akonadicomponents PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS akonadicomponents DESTINATION ${LIB_INSTALL_DIR})

########### install files ###############

install( FILES
	agenttypeview.h
	profileview.h
	DESTINATION ${INCLUDE_INSTALL_DIR}/libakonadi/components
)
