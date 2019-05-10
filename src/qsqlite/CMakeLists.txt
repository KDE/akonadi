set(QSqlite_SRCS
    src/sqlite_blocking.cpp
    src/qsql_sqlite.cpp
    src/smain.cpp
)

message(STATUS "Building QSQLITE3 driver")

set(QSQLITE_INSTALL_PREFIX "${PLUGIN_INSTALL_DIR}/sqldrivers")

include_directories(
    ${Qt5Core_PRIVATE_INCLUDE_DIRS}
    ${Qt5Sql_PRIVATE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLITE_INCLUDE_DIR}
)

add_library(qsqlite3 SHARED ${QSqlite_SRCS} ${QSqlite_MOC_SRCS})

set_target_properties(qsqlite3 PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/sqldrivers")

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
