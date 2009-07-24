project(QSqlite)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

find_package(Sqlite REQUIRED)

set(QSqlite_SRCS
  src/qsql_sqlite.cpp
  src/smain.cpp
)

# another list, this time it includes all header files that should be treated with moc
SET(QSqlite_MOC_HDRS
  src/qsql_sqlite.h
)

add_definitions(-Wall)
add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)

set(QT_USE_QTSQL TRUE)

find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})

qt4_wrap_cpp(QSqlite_MOC_SRCS ${QSqlite_MOC_HDRS})

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)

add_library(qsqlite3 SHARED ${QSqlite_SRCS} ${QSqlite_MOC_SRCS})

target_link_libraries(qsqlite3 ${QT_QTCORE_LIBRARY} ${QT_QTSQL_LIBRARY} ${SQLITE_LIBRARIES})

# -DCMAKE_INSTALL_PREFIX=/d/4.x/install
INSTALL(TARGETS qsqlite3
  LIBRARY DESTINATION lib/kde4/plugins/sqldrivers
)
