/*
    Copyright (c) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
    Authors: David Faure <david.faure@kdab.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include <QString>
#include <QStandardItem>
#include <QSignalSpy>

namespace TestModelHelpers
{

// Prepares one row for a QStandardItemModel
inline QList<QStandardItem *> makeStandardItems(const QStringList &texts)
{
    QList<QStandardItem *> items;
    foreach (const QString &txt, texts) {
        items << new QStandardItem(txt);
    }
    return items;
}

// Extracts a full row from a model as a string
// Works best if every cell contains only one character
inline QString extractRowTexts(QAbstractItemModel *model, int row, const QModelIndex &parent = QModelIndex())
{
    QString result;
    const int colCount = model->columnCount();
    for (int col = 0; col < colCount; ++col) {
        const QString txt = model->index(row, col, parent).data().toString();
        result += txt.isEmpty() ? QStringLiteral(" ") : txt;
    }
    return result;
}

// Extracts all headers
inline QString extractHorizontalHeaderTexts(QAbstractItemModel *model)
{
    QString result;
    const int colCount = model->columnCount();
    for (int col = 0; col < colCount; ++col) {
        const QString txt = model->headerData(col, Qt::Horizontal).toString();
        result += txt.isEmpty() ? QStringLiteral(" ") : txt;
    }
    return result;
}

inline QString rowSpyToText(const QSignalSpy &spy)
{
    if (!spy.isValid()) {
        return QStringLiteral("THE SIGNALSPY IS INVALID!");
    }
    QString str;
    for (int i = 0; i < spy.count(); ++i) {
        str += spy.at(i).at(1).toString() + QLatin1Char(',') + spy.at(i).at(2).toString();
        if (i + 1 < spy.count()) {
            str += QLatin1Char(';');
        }
    }
    return str;
}

}

