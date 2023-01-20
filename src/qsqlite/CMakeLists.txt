set(QSqlite_SRCS
    src/sqlite_blocking.cpp src/sqlite_blocking.h
    src/qsql_sqlite.cpp src/qsql_sqlite.h
    src/smain.cpp
)

message(STATUS "Building QSQLITE3 driver")

set(QSQLITE_INSTALL_PREFIX "${KDE_INSTALL_PLUGINDIR}/sqldrivers")

# TODO KF6: Use Qt6::CorePrivate and Qt6::SqlPrivate
include_directories(
    ${Qt${QT_MAJOR_VERSION}Core_PRIVATE_INCLUDE_DIRS}
    ${Qt${QT_MAJOR_VERSION}Sql_PRIVATE_INCLUDE_DIRS}
)

add_library(qsqlite3 SHARED ${QSqlite_SRCS} ${QSqlite_MOC_SRCS})

set_target_properties(qsqlite3 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/sqldrivers")

target_link_libraries(qsqlite3
    Qt::Core
    Qt::Sql
    Sqlite::Sqlite
)

INSTALL(TARGETS qsqlite3
  RUNTIME DESTINATION ${QSQLITE_INSTALL_PREFIX}
  LIBRARY DESTINATION ${QSQLITE_INSTALL_PREFIX}
  ARCHIVE DESTINATION ${QSQLITE_INSTALL_PREFIX}
)
