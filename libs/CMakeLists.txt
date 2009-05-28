set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII ${_ENABLE_EXCEPTIONS}" )

# libakonadiprotocolinternals
set( akonadiprotocolinternals_srcs
  imapparser.cpp
  imapset.cpp
  notificationmessage.cpp
  xdgbasedirs.cpp
)

add_library( akonadiprotocolinternals SHARED ${akonadiprotocolinternals_srcs} )

target_link_libraries( akonadiprotocolinternals ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY})
set_target_properties( akonadiprotocolinternals PROPERTIES
                        VERSION ${AKONADI_VERSION}
                        SOVERSION ${AKONADI_VERSION_MAJOR}
                        DEFINE_SYMBOL MAKE_AKONADIPROTOCOLINTERNALS_LIB
                      )

install( TARGETS akonadiprotocolinternals ${INSTALL_TARGETS_DEFAULT_ARGS} )

install( FILES
  akonadiprotocolinternals_export.h
  imapparser_p.h
  imapset_p.h
  notificationmessage_p.h
  protocol_p.h
  xdgbasedirs_p.h
  DESTINATION ${CMAKE_INSTALL_PREFIX}/include/akonadi/private
)

add_subdirectory( tests )

