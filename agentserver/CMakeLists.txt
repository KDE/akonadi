set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS}" )
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${Akonadi_SOURCE_DIR})

set( akonadi_agent_server_srcs
  ${AKONADI_SHARED_SOURCES}
  agentserver.cpp
  agentthread.cpp
  main.cpp
)

automoc4_add_executable(akonadi_agent_server ${akonadi_agent_server_srcs})

target_link_libraries(akonadi_agent_server
  ${QT_QTCORE_LIBRARY}
  ${QT_QTDBUS_LIBRARY}
  ${AKONADI_SYSTEM_LIBS}
  ${AKONADI_PROTOCOLINTERNALS_LIBS}
)

if(NOT WINCE)
    target_link_libraries(akonadi_agent_server ${Boost_PROGRAM_OPTIONS_LIBRARY})
endif(NOT WINCE)

install(TARGETS akonadi_agent_server DESTINATION ${BIN_INSTALL_DIR})
