# SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
# SPDX-License-Identifier: BSD-2-Clause

add_executable(akonadiagentconfigdialog main.cpp)

target_link_libraries(
    akonadiagentconfigdialog
    PRIVATE
        KPim6::AkonadiWidgets
        KF6::I18n
        KF6::CoreAddons
        Qt::Widgets
)

install(
    TARGETS
        akonadiagentconfigdialog
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)
install(FILES org.kde.akonadi.configdialog.desktop DESTINATION ${KDE_INSTALL_APPDIR})
