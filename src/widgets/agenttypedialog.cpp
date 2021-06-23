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
#include <KFilterProxySearchLine>
#include <KSharedConfig>
#include <QLineEdit>

#include <QDialogButtonBox>
#include <QPushButton>

using namespace Akonadi;

class Q_DECL_HIDDEN AgentTypeDialog::Private
{
public:
    explicit Private(AgentTypeDialog *qq)
        : q(qq)
    {
    }
    void readConfig();
    void writeConfig() const;
    void slotSearchAgentType(const QString &str);
    AgentTypeWidget *Widget = nullptr;
    AgentType agentType;
    AgentTypeDialog *const q;
};

void AgentTypeDialog::Private::writeConfig() const
{
    KConfigGroup group(KSharedConfig::openStateConfig(), "AgentTypeDialog");
    group.writeEntry("Size", q->size());
}

void AgentTypeDialog::Private::readConfig()
{
    KConfigGroup group(KSharedConfig::openStateConfig(), "AgentTypeDialog");
    const QSize sizeDialog = group.readEntry("Size", QSize(460, 320));
    if (sizeDialog.isValid()) {
        q->resize(sizeDialog);
    }
}

void AgentTypeDialog::Private::slotSearchAgentType(const QString &str)
{
    Widget->agentFilterProxyModel()->setFilterRegularExpression(str);
}

AgentTypeDialog::AgentTypeDialog(QWidget *parent)
    : QDialog(parent)
    , d(new Private(this))
{
    auto layout = new QVBoxLayout(this);

    d->Widget = new Akonadi::AgentTypeWidget(this);
    connect(d->Widget, &AgentTypeWidget::activated, this, &AgentTypeDialog::accept);

    auto searchLine = new QLineEdit(this);
    layout->addWidget(searchLine);
    searchLine->setClearButtonEnabled(true);
    connect(searchLine, &QLineEdit::textChanged, this, [this](const QString &str) {
        d->slotSearchAgentType(str);
    });

    layout->addWidget(d->Widget);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AgentTypeDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AgentTypeDialog::reject);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return); // NOLINT(bugprone-suspicious-enum-usage)
    layout->addWidget(buttonBox);
    d->readConfig();

    searchLine->setFocus();
}

AgentTypeDialog::~AgentTypeDialog()
{
    d->writeConfig();
    delete d;
}

void AgentTypeDialog::done(int result)
{
    if (result == Accepted) {
        d->agentType = d->Widget->currentAgentType();
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
    return d->Widget->agentFilterProxyModel();
}
