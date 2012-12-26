project(akonadi_rds)

#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS}" )
set(AKONADI_PROTOCOLINTERNALS_LIBS ${akonadiprotocolinternals_LIB_DEPENDS} akonadiprotocolinternals)

########### next target ###############

set(akonadi_rds_srcs
  bridgeserver.cpp
  bridgeconnection.cpp
  main.cpp
)

add_executable(akonadi_rds ${akonadi_rds_srcs})

target_link_libraries(akonadi_rds akonadi_shared ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${AKONADI_PROTOCOLINTERNALS_LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})

install(TARGETS akonadi_rds DESTINATION ${BIN_INSTALL_DIR})

