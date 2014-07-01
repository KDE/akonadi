set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS}")

configure_file(akonadiprotocolinternals_export.h.in ${Akonadi_BINARY_DIR}/akonadiprotocolinternals_export.h)

# libakonadiprotocolinternals
set(akonadiprotocolinternals_srcs
  imapparser.cpp
  imapset.cpp
  notificationmessage.cpp
  notificationmessagev2.cpp
  notificationmessagev3.cpp
  xdgbasedirs.cpp
)

add_library(akonadiprotocolinternals ${LIBRARY_TYPE} ${akonadiprotocolinternals_srcs})

target_link_libraries(akonadiprotocolinternals ${QT_QTCORE_LIBRARY} ${QT_QTDBUS_LIBRARY})
set_target_properties(akonadiprotocolinternals PROPERTIES
                        VERSION ${AKONADI_VERSION}
                        SOVERSION ${AKONADI_SOVERSION}
                        DEFINE_SYMBOL MAKE_AKONADIPROTOCOLINTERNALS_LIB
                      )

install(TARGETS akonadiprotocolinternals EXPORT akonadiLibraryTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
  ${Akonadi_BINARY_DIR}/akonadiprotocolinternals_export.h
  imapparser_p.h
  imapset_p.h
  notificationmessage_p.h
  notificationmessagev2_p.h
  notificationmessagev3_p.h
  protocol_p.h
  xdgbasedirs_p.h
  capabilities_p.h
  DESTINATION ${INCLUDE_INSTALL_DIR}/akonadi/private
)

add_subdirectory(tests)

