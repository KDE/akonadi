// SPDX-FileCopyrightText: 2025 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <Akonadi/AgentConfigurationDialog>
#include <Akonadi/AgentInstance>
#include <Akonadi/AgentManager>

#include <KAboutData>
#include <KLocalizedString>
#include <KMessageBox>

#include <QApplication>
#include <QCommandLineParser>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData about(u"akonadiagentconfigdialog"_s,
                     i18n("Configuration Dialog"),
                     u"1.0"_s,
                     i18n("Allows to configure an agent or resource"),
                     KAboutLicense::GPL_V2,
                     i18n("© 2025 Carl Schwan <carl@carlschwan.eu>"));
    about.setupCommandLine(&parser);

    parser.addPositionalArgument(u"identifier"_s, i18n("The agent identifier"));

    parser.process(app);

    const auto positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        KMessageBox::error(nullptr, i18n("Required identifier argument missing."));
    }
    auto identifier = positionalArguments.at(0);
    if (identifier.startsWith(u"akonadi:")) { // KIO::ApplicationLauncherJob uses Urls
        identifier = identifier.mid(u"akonadi:"_s.size());
    }

    const auto manager = Akonadi::AgentManager::self();
    const auto instance = manager->instance(identifier);

    if (instance.isValid()) {
        auto dlg = new Akonadi::AgentConfigurationDialog(instance);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    } else {
        KMessageBox::error(nullptr, i18n("No such agent found \"%1\"", identifier));
    }

    return app.exec();
}
