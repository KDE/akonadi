set(QSqlite_SRCS
  src/sqlite_blocking.cpp
  src/qsql_sqlite.cpp
  src/smain.cpp
)

message(STATUS "Building QSQLITE3 driver")

if(INSTALL_QSQLITE_IN_QT_PREFIX)
  set(QSQLITE_INSTALL_PREFIX "${QT_PLUGINS_DIR}/sqldrivers")
else()
  set(QSQLITE_INSTALL_PREFIX "${LIB_INSTALL_DIR}/plugins/sqldrivers")
endif()

add_definitions(-Wall)
add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SQLITE_INCLUDE_DIR}
)

if(AKONADI_STATIC_SQLITE)
  add_definitions(-DQT_STATICPLUGIN)
  add_library(qsqlite3 STATIC ${QSqlite_SRCS} ${QSqlite_MOC_SRCS})
else()
  add_library(qsqlite3 SHARED ${QSqlite_SRCS} ${QSqlite_MOC_SRCS})
endif()

target_link_libraries(qsqlite3 ${QT_QTCORE_LIBRARY} ${QT_QTSQL_LIBRARY})

if(SQLITE_LINK_STATIC)
  message(STATUS "SQlite ${SQLITE_STATIC_LIBRARIES}")
  target_link_libraries(qsqlite3 ${SQLITE_STATIC_LIBRARIES})
else()
  message(STATUS "SQlite ${SQLITE_LIBRARIES}")
  target_link_libraries(qsqlite3 ${SQLITE_LIBRARIES})
endif()

INSTALL(TARGETS qsqlite3
  RUNTIME DESTINATION ${QSQLITE_INSTALL_PREFIX}
  LIBRARY DESTINATION ${QSQLITE_INSTALL_PREFIX}
  ARCHIVE DESTINATION ${QSQLITE_INSTALL_PREFIX}
)
