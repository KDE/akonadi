include_directories(${MYSQL_INCLUDE_DIR})

set(mysql_embedded_plugin_PART_SRCS main.cpp qsql_mysql_embedded.cpp )

kde4_automoc(${mysql_embedded_plugin_PART_SRCS})

kde4_add_plugin(qsql_mysql_embedded ${mysql_embedded_plugin_PART_SRCS})
set_target_properties(qsql_mysql_embedded PROPERTIES OUTPUT_NAME libqsqlmysqlembedded)

# MYSQL_LIBRARIES is necessary since it defines yaSSL_CTX_free while MYSQL_EMBEDDED_LIBRARIES doesn't (on kubuntu edgy at least)
target_link_libraries(qsql_mysql_embedded ${QT_QTCORE_LIBRARY} ${QT_QTSQL_LIBRARY}
                                          ${MYSQL_EMBEDDED_LIBRARIES} ${OPENSSL_LIBRARIES} crypto crypt nsl z)

install(TARGETS qsql_mysql_embedded DESTINATION ${PLUGIN_INSTALL_DIR}/plugins/sqldrivers)
