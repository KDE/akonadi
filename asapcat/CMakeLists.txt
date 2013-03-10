project(asapcat)

#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_ENABLE_EXCEPTIONS}" )
set(AKONADI_PROTOCOLINTERNALS_LIBS ${akonadiprotocolinternals_LIB_DEPENDS} akonadiprotocolinternals)

set(asapcat_srcs
  main.cpp
  session.cpp
)

add_executable(asapcat ${asapcat_srcs})

target_link_libraries(asapcat akonadi_shared ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${AKONADI_PROTOCOLINTERNALS_LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})

install(TARGETS asapcat DESTINATION ${BIN_INSTALL_DIR})
