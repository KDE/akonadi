/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "agenttypedialog.h"
#include "agentfilterproxymodel.h"

#include <KConfig>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QLineEdit>

#include <QDialogButtonBox>
#include <QPushButton>

using namespace Akonadi;
namespace
{
static const char myAgentTypeDialogGroupName[] = "AgentTypeDialog";
}
class Akonadi::AgentTypeDialogPrivate
{
public:
    explicit AgentTypeDialogPrivate(AgentTypeDialog *qq)
        : q(qq)
    {
    }
    void readConfig();
    void writeConfig() const;
    void slotSearchAgentType(const QString &str);
    AgentTypeWidget *widget = nullptr;
    AgentType agentType;
    AgentTypeDialog *const q;
};

void AgentTypeDialogPrivate::writeConfig() const
{
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myAgentTypeDialogGroupName));
    group.writeEntry("Size", q->size());
}

void AgentTypeDialogPrivate::readConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), QLatin1StringView(myAgentTypeDialogGroupName));
    const QSize sizeDialog = group.readEntry("Size", QSize(460, 320));
    if (sizeDialog.isValid()) {
        q->resize(sizeDialog);
    }
}

void AgentTypeDialogPrivate::slotSearchAgentType(const QString &str)
{
    widget->agentFilterProxyModel()->setFilterRegularExpression(str);
}

AgentTypeDialog::AgentTypeDialog(QWidget *parent)
    : QDialog(parent)
    , d(new AgentTypeDialogPrivate(this))
{
    setWindowTitle(i18nc("@title:window", "Configure Account"));
    auto layout = new QVBoxLayout(this);

    d->widget = new Akonadi::AgentTypeWidget(this);
    connect(d->widget, &AgentTypeWidget::activated, this, &AgentTypeDialog::accept);

    auto searchLine = new QLineEdit(this);
    layout->addWidget(searchLine);
    searchLine->setPlaceholderText(i18nc("@item:textbox", "Search types"));
    searchLine->setClearButtonEnabled(true);
    connect(searchLine, &QLineEdit::textChanged, this, [this](const QString &str) {
        d->slotSearchAgentType(str);
    });

    layout->addWidget(d->widget);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AgentTypeDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AgentTypeDialog::reject);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setEnabled(false);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return); // NOLINT(bugprone-suspicious-enum-usage)
    connect(d->widget, &Akonadi::AgentTypeWidget::currentChanged, this, [okButton](const Akonadi::AgentType &current, const Akonadi::AgentType &previous) {
        Q_UNUSED(previous);
        okButton->setEnabled(current.isValid());
    });
    layout->addWidget(buttonBox);
    d->readConfig();

    searchLine->setFocus();
}

AgentTypeDialog::~AgentTypeDialog()
{
    d->writeConfig();
}

void AgentTypeDialog::done(int result)
{
    if (result == Accepted) {
        d->agentType = d->widget->currentAgentType();
    } else {
        d->agentType = AgentType();
    }

    QDialog::done(result);
}

AgentType AgentTypeDialog::agentType() const
{
    return d->agentType;
}

AgentFilterProxyModel *AgentTypeDialog::agentFilterProxyModel() const
{
    return d->widget->agentFilterProxyModel();
}

#include "moc_agenttypedialog.cpp"
