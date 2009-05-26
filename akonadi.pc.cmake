prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=@LIB_INSTALL_DIR@
includedir=@CMAKE_INSTALL_PREFIX@/include

Name: Akonadi
Description: Akonadi server and infrastructure needed to build client libraries and applications
Version: @AKONADI_VERSION_STRING@
Requires: QtCore QtSql QtDBus
Libs: -L${libdir} -lakonadiprotocolinternals
Cflags: -I${includedir}
