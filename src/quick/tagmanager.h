// SPDX-FileCopyrightText: 2021 Claudio Cambra <claudio.cambra@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Akonadi/Monitor>
#include <Akonadi/TagModel>
#include <QObject>
#include <QSortFilterProxyModel>
#include <qqmlregistration.h>

class TagManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QSortFilterProxyModel *tagModel READ tagModel NOTIFY tagModelChanged)

public:
    explicit TagManager(QObject *parent = nullptr);
    ~TagManager() override = default;

    QSortFilterProxyModel *tagModel() const;
    Q_INVOKABLE void createTag(const QString &name);
    Q_INVOKABLE void renameTag(Akonadi::Tag tag, const QString &newName);
    Q_INVOKABLE void deleteTag(Akonadi::Tag tag);

Q_SIGNALS:
    void tagModelChanged();

private:
    QSortFilterProxyModel *const m_tagModel;
};
