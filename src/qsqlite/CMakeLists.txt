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

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLITE_INCLUDE_DIR}
)

add_library(qsqlite3 SHARED ${QSqlite_SRCS} ${QSqlite_MOC_SRCS})

target_link_libraries(qsqlite3
    Qt5::Core
    Qt5::Sql
    ${SQLITE_LIBRARIES}
)

INSTALL(TARGETS qsqlite3
  RUNTIME DESTINATION ${QSQLITE_INSTALL_PREFIX}
  LIBRARY DESTINATION ${QSQLITE_INSTALL_PREFIX}
  ARCHIVE DESTINATION ${QSQLITE_INSTALL_PREFIX}
)
