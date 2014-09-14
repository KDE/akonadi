/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2008 Omat Holding B.V. <info@omat.nl>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "agenttypedialog.h"
#include "agentfilterproxymodel.h"

#include <QVBoxLayout>
#include <KConfig>

#include <kfilterproxysearchline.h>
#include <klineedit.h>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QDialogButtonBox>
#include <QPushButton>

using namespace Akonadi;

class AgentTypeDialog::Private
{
public:
    Private(AgentTypeDialog *qq)
        : q(qq)
    {

    }
    void readConfig();
    void writeConfig();
    AgentTypeWidget *Widget;
    AgentType agentType;
    AgentTypeDialog *q;
};

void AgentTypeDialog::Private::writeConfig()
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
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    d->Widget = new Akonadi::AgentTypeWidget;
    connect(d->Widget, &AgentTypeWidget::activated, this, &AgentTypeDialog::accept);

    KFilterProxySearchLine *searchLine = new KFilterProxySearchLine;
    layout->addWidget(searchLine);
    searchLine->setProxy(d->Widget->agentFilterProxyModel());

    layout->addWidget(d->Widget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &AgentTypeDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &AgentTypeDialog::reject);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    layout->addWidget(buttonBox);
    d->readConfig();

    searchLine->lineEdit()->setFocus();
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
