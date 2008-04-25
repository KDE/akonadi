prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=@LIB_INSTALL_DIR@
includedir=@INCLUDE_INSTALL_DIR@

Name: Akonadi
Description: Akonadi server and infrastructure needed to build client libraries and applications
Version: @AKONADI_LIB_VERSION_STRING@
Requires: QtCore QtSql QtDBus
Libs: -L${libdir} -lakonadiprotocolinternals
Cflags: -I${includedir}
