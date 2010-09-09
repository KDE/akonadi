/* This file contains all the paths that change when changing the installation prefix */

#define AKONADIPREFIX "${CMAKE_INSTALL_PREFIX}"
#define AKONADIDATA   "${SHARE_INSTALL_PREFIX}"
#define AKONADICONFIG "${CONFIG_INSTALL_DIR}"
// the below is hardcoded to what FindKDE4Internal.cmake sets it to, since we have no way of finding that
#define AKONADIBUNDLEPATH "${AKONADI_BUNDLE_PATH}"
