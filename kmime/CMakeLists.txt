include_directories(
	${CMAKE_SOURCE_DIR}/
	${CMAKE_SOURCE_DIR}/akonadi
	${QT_QTDBUS_INCLUDE_DIR}
        ${Boost_INCLUDE_DIR}
)

if(NOT MSVC)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )
endif(NOT MSVC)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set( kmimeakonadi_LIB_SRC
  messagemodel.cpp
)

kde4_automoc(${kmimeakonadi_LIB_SRC})
kde4_add_library( kmimeakonadi SHARED ${kmimeakonadi_LIB_SRC} )

target_link_libraries( kmimeakonadi akonadi ${KDE4_KMIME_LIBS} )
set_target_properties( kmimeakonadi PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS kmimeakonadi DESTINATION ${LIB_INSTALL_DIR})

########### install files ###############

install( FILES
  messagemodel.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/libakonadi/kmime
)
