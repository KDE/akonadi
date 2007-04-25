include_directories(
	${CMAKE_SOURCE_DIR}/akonadi
	${CMAKE_SOURCE_DIR}/akonadi/libakonadi
	${QT_QTDBUS_INCLUDE_DIR}
)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )


########### next target ###############

set( knutresource_SRCS
  knutresource.cpp
  main.cpp
)

install( FILES knutresource.desktop knut.svg DESTINATION "${CMAKE_INSTALL_PREFIX}/share/apps/akonadi/agents" )

kde4_automoc(${knutresource_SRCS})

kde4_add_executable(akonadi_knut_resource RUN_UNINSTALLED ${knutresource_SRCS})

target_link_libraries(akonadi_knut_resource akonadi ${QT_QTCORE_LIBRARIES} ${QT_QTDBUS_LIBRARIES} kabc kcal)

install(TARGETS akonadi_knut_resource DESTINATION ${BIN_INSTALL_DIR})






