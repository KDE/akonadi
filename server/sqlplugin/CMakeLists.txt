include_directories(${MYSQL_INCLUDE_DIR})

find_library(NSL_LIB   NAMES nsl)
find_library(CRYPT_LIB NAMES crypt)
  
if (NSL_LIB)
  set(NSL_LIBRARY ${NSL_LIB})
endif (NSL_LIB)
  
if (CRYPT_LIB)
  set(CRYPT_LIBRARY ${CRYPT_LIB})
endif (CRYPT_LIB)
  
mark_as_advanced(NSL_LIBRARY CRYPT_LIBRARY)

set(mysql_embedded_plugin_PART_SRCS main.cpp qsql_mysql_embedded.cpp )


kde4_add_plugin(qsql_mysql_embedded ${mysql_embedded_plugin_PART_SRCS})
set_target_properties(qsql_mysql_embedded PROPERTIES OUTPUT_NAME libqsqlmysqlembedded)

# MYSQL_LIBRARIES is necessary since it defines yaSSL_CTX_free while MYSQL_EMBEDDED_LIBRARIES doesn't (on kubuntu edgy at least)
target_link_libraries(qsql_mysql_embedded ${QT_QTCORE_LIBRARY} ${QT_QTSQL_LIBRARY}
                                          ${MYSQL_EMBEDDED_LIBRARIES} ${MYSQL_LIBRARIES} ${OPENSSL_LIBRARIES} crypto ${CRYPT_LIBRARY} ${NSL_LIBRARY} z)

install(TARGETS qsql_mysql_embedded DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/sqldrivers)
