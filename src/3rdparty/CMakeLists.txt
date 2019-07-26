# Don't add Optional subdir, it has install() rules that we don't want
install(FILES Optional/optional.hpp DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/AkonadiCore/)
