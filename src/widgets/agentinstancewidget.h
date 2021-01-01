/*
    SPDX-FileCopyrightText: 2006-2008 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_AGENTINSTANCEWIDGET_H
#define AKONADI_AGENTINSTANCEWIDGET_H

#include "akonadiwidgets_export.h"

#include <QWidget>

class QAbstractItemView;
namespace Akonadi
{

class AgentInstance;
class AgentFilterProxyModel;

/**
 * @short Provides a widget that lists all available agent instances.
 *
 * The widget is listening on the dbus for changes, so the
 * widget is updated automatically as soon as new agent instances
 * are added to or removed from the system.
 *
 * @code
 *
 * MyWidget::MyWidget( QWidget *parent )
 *   : QWidget( parent )
 * {
 *   QVBoxLayout *layout = new QVBoxLayout( this );
 *
 *   mAgentInstanceWidget = new Akonadi::AgentInstanceWidget( this );
 *   layout->addWidget( mAgentInstanceWidget );
 *
 *   connect( mAgentInstanceWidget, SIGNAL(doubleClicked(Akonadi::AgentInstance)),
 *            this, SLOT(slotInstanceSelected(Akonadi::AgentInstance)) );
 * }
 *
 * ...
 *
 * MyWidget::slotInstanceSelected( Akonadi::AgentInstance &instance )
 * {
 *   qCDebug(AKONADIWIDGETS_LOG) << "Selected instance" << instance.name();
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADIWIDGETS_EXPORT AgentInstanceWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Creates a new agent instance widget.
     *
     * @param parent The parent widget.
     */
    explicit AgentInstanceWidget(QWidget *parent = nullptr);

    /**
     * Destroys the agent instance widget.
     */
    ~AgentInstanceWidget();

    /**
     * Returns the current agent instance or an invalid agent instance
     * if no agent instance is selected.
     */
    Q_REQUIRED_RESULT AgentInstance currentAgentInstance() const;

    /**
     * Returns the selected agent instances.
     * @since 4.5
     */
    Q_REQUIRED_RESULT QVector<AgentInstance> selectedAgentInstances() const;

    /**
     * Returns the agent filter proxy model, use this to filter by
     * agent mimetype or capabilities.
     */
    Q_REQUIRED_RESULT AgentFilterProxyModel *agentFilterProxyModel() const;

    /**
     * Returns the view used in the widget.
     * @since 4.5
     */
    Q_REQUIRED_RESULT QAbstractItemView *view() const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever the current agent instance changes.
     *
     * @param current The current agent instance.
     * @param previous The previous agent instance.
     */
    void currentChanged(const Akonadi::AgentInstance &current, const Akonadi::AgentInstance &previous);

    /**
     * This signal is emitted whenever there is a double click on an agent instance.
     *
     * @param current The current agent instance.
     */
    void doubleClicked(const Akonadi::AgentInstance &current);

    /**
     * This signal is emitted whenever there is a click on an agent instance.
     *
     * @param current The current agent instance.
     * @since 4.9.1
     */
    void clicked(const Akonadi::AgentInstance &current);

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
