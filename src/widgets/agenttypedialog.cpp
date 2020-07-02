/*
    SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2008 Omat Holding B.V. <info@omat.nl>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "agenttypedialog.h"
#include "agentfilterproxymodel.h"

#include <QVBoxLayout>
#include <KConfig>

#include <KFilterProxySearchLine>
#include <QLineEdit>
#include <KSharedConfig>
#include <KConfigGroup>

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
    AgentTypeWidget *Widget = nullptr;
    AgentType agentType;
    AgentTypeDialog *q = nullptr;
};

void AgentTypeDialog::Private::writeConfig() const
{
    KConfigGroup group(KSharedConfig::openConfig(), "AgentTypeDialog");
    group.writeEntry("Size", q->size());
}

void AgentTypeDialog::Private::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "AgentTypeDialog");
    const QSize sizeDialog = group.readEntry("Size", QSize(460, 320));
    if (sizeDialog.isValid()) {
        q->resize(sizeDialog);
    }
}

AgentTypeDialog::AgentTypeDialog(QWidget *parent)
    : QDialog(parent)
    , d(new Private(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    d->Widget = new Akonadi::AgentTypeWidget(this);
    connect(d->Widget, &AgentTypeWidget::activated, this, &AgentTypeDialog::accept);

    QLineEdit *searchLine = new QLineEdit(this);
    layout->addWidget(searchLine);
    searchLine->setClearButtonEnabled(true);
    connect(searchLine, &QLineEdit::textChanged, d->Widget->agentFilterProxyModel(), &AgentFilterProxyModel::setFilterFixedString);

    layout->addWidget(d->Widget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
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
