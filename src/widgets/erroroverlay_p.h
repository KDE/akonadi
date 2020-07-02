/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ERROROVERLAY_P_H
#define AKONADI_ERROROVERLAY_P_H

#include "servermanager.h"

#include <QPointer>
#include <QWidget>

namespace Ui
{
class ErrorOverlay;
}

namespace Akonadi
{

/**
 * @internal
 * Overlay widget to block Akonadi-dependent widgets if the Akonadi server
 * is unavailable.
 * @todo handle initial parent == 0 case correctly, reparent later and hide as long as parent widget is 0
 * @todo fix hiding in dock widget tabs
 */
class ErrorOverlay : public QWidget
{
    Q_OBJECT
public:
    /**
     * Create an overlay widget for @p baseWidget.
     * @p baseWidget must not be null.
     * @p parent must not be equal to @p baseWidget
     */
    explicit ErrorOverlay(QWidget *baseWidget, QWidget *parent = nullptr);
    ~ErrorOverlay() override;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    void reposition();

private Q_SLOTS:
    void startClicked();
    void quitClicked();
    void selfTestClicked();
    void serverStateChanged(Akonadi::ServerManager::State state);

private:
    QPointer<QWidget> mBaseWidget;
    bool mPreviousState;
    bool mOverlayActive;
    bool mBaseWidgetIsParent;
    QScopedPointer<Ui::ErrorOverlay> ui;
};

}

#endif
