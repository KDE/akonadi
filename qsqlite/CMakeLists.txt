set(QSqlite_SRCS
  src/sqlite_blocking.cpp
  src/qsql_sqlite.cpp
  src/smain.cpp
)

# another list, this time it includes all header files that should be treated with moc
SET(QSqlite_MOC_HDRS
  src/qsql_sqlite.h
)

message(STATUS SQlite ${SQLITE_LIBRARIES})
message(STATUS "Building QSQLITE3 driver")
message(STATUS "Make sure that you add " ${CMAKE_INSTALL_PREFIX}/lib/kde4/plugins/sqldrivers " to QT_PLUGIN_PATH")

add_definitions(-Wall)
add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)

qt4_wrap_cpp(QSqlite_MOC_SRCS ${QSqlite_MOC_HDRS})

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SQLITE_INCLUDE_DIR}
)

add_library(qsqlite3 SHARED ${QSqlite_SRCS} ${QSqlite_MOC_SRCS})

target_link_libraries(qsqlite3 ${QT_QTCORE_LIBRARY} ${QT_QTSQL_LIBRARY} ${SQLITE_LIBRARIES})

# -DCMAKE_INSTALL_PREFIX=/d/4.x/install
INSTALL(TARGETS qsqlite3
  LIBRARY DESTINATION lib/kde4/plugins/sqldrivers
)
