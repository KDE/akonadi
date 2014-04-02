/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "geoeditwidget.h"

#include "autoqpointer_p.h"

#include <kabc/addressee.h>
#include <kabc/geo.h>
#include <kcombobox.h>
#include <klocale.h>
#include <klocalizedstring.h>


#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>

class GeoMapWidget : public QWidget
{
public:
    GeoMapWidget(QWidget *parent = 0)
        : QWidget(parent)
    {
        mWorld = QPixmap(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("akonadi/contact/pics/world.jpg")));

        setAttribute(Qt::WA_NoSystemBackground, true);
        setFixedSize(400, 200);

        update();
    }

    void setCoordinates(const KABC::Geo &coordinates)
    {
        mCoordinates = coordinates;

        update();
    }

protected:
    virtual void paintEvent(QPaintEvent *)
    {
        QPainter p;
        p.begin(this);
        p.setPen(QColor(255, 0, 0));
        p.setBrush(QColor(255, 0, 0));

        p.drawPixmap(0, 0, mWorld);

        if (mCoordinates.isValid()) {
            const double latMid = height() / 2;
            const double longMid = width() / 2;
            const double latOffset = (mCoordinates.latitude() * latMid) / 90;
            const double longOffset = (mCoordinates.longitude() * longMid) / 180;

            const int x = (int)(longMid + longOffset);
            const int y = (int)(latMid - latOffset);
            p.drawEllipse(x, y, 4, 4);
        }

        p.end();
    }

private:
    QPixmap mWorld;
    KABC::Geo mCoordinates;
};

GeoEditWidget::GeoEditWidget(QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);

    mMap = new GeoMapWidget;
    layout->addWidget(mMap, 0, 0, 1, 4, Qt::AlignCenter | Qt::AlignVCenter);

    QLabel *label = new QLabel(i18nc("@label", "Latitude:"));
    label->setAlignment(Qt::AlignRight);
    layout->addWidget(label, 1, 0);

    mLatitudeLabel = new QLabel;
    layout->addWidget(mLatitudeLabel, 1, 1);

    label = new QLabel(i18nc("@label", "Longitude:"));
    label->setAlignment(Qt::AlignRight);
    layout->addWidget(label, 1, 2);

    mLongitudeLabel = new QLabel;
    layout->addWidget(mLongitudeLabel, 1, 3);

    mChangeButton = new QPushButton(i18nc("@label Change the coordinates", "Change..."));
    layout->addWidget(mChangeButton, 2, 0, 1, 4, Qt::AlignRight);

    layout->setRowStretch(3, 1);

    connect(mChangeButton, SIGNAL(clicked()), SLOT(changeClicked()));

    updateView();
}

GeoEditWidget::~GeoEditWidget()
{
}

void GeoEditWidget::loadContact(const KABC::Addressee &contact)
{
    mCoordinates = contact.geo();
    updateView();
}

void GeoEditWidget::storeContact(KABC::Addressee &contact) const
{
    contact.setGeo(mCoordinates);
}

void GeoEditWidget::setReadOnly(bool readOnly)
{
    mChangeButton->setEnabled(!readOnly);
}

void GeoEditWidget::updateView()
{
    if (!mCoordinates.isValid()) {
        mLatitudeLabel->setText(i18nc("@label Coordinates are not available", "n/a"));
        mLongitudeLabel->setText(i18nc("@label Coordinates are not available", "n/a"));
    } else {
        mLatitudeLabel->setText(i18nc("@label The formatted coordinates", "%1 %2", mCoordinates.latitude(), QChar(176)));
        mLongitudeLabel->setText(i18nc("@label The formatted coordinates", "%1 %2", mCoordinates.longitude(), QChar(176)));
    }
    mMap->setCoordinates(mCoordinates);
}

void GeoEditWidget::changeClicked()
{
    AutoQPointer<GeoDialog> dlg = new GeoDialog(mCoordinates, this);
    if (dlg->exec()) {
        mCoordinates = dlg->coordinates();
        updateView();
    }
}

static double calculateCoordinate(const QString &coordinate)
{
    int neg;
    int d = 0, m = 0, s = 0;
    QString str = coordinate;

    neg = str.left(1) == QLatin1String("-");
    str.remove(0, 1);

    switch (str.length()) {
    case 4:
        d = str.left(2).toInt();
        m = str.mid(2).toInt();
        break;
    case 5:
        d = str.left(3).toInt();
        m = str.mid(3).toInt();
        break;
    case 6:
        d = str.left(2).toInt();
        m = str.mid(2, 2).toInt();
        s = str.right(2).toInt();
        break;
    case 7:
        d = str.left(3).toInt();
        m = str.mid(3, 2).toInt();
        s = str.right(2).toInt();
        break;
    default:
        break;
    }

    if (neg) {
        return - (d + m / 60.0 + s / 3600.0);
    } else {
        return d + m / 60.0 + s / 3600.0;
    }
}

GeoDialog::GeoDialog(const KABC::Geo &coordinates, QWidget *parent)
    : KDialog(parent)
    , mCoordinates(coordinates)
{
    KLocalizedString::insertCatalog(QStringLiteral("timezones4"));
    setCaption(i18nc("@title:window", "Coordinate Selection"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);
    setModal(true);

    QFrame *page = new QFrame(this);
    setMainWidget(page);

    QVBoxLayout *layout = new QVBoxLayout(page);

    mCityCombo = new KComboBox(page);
    layout->addWidget(mCityCombo);

    QGroupBox *decimalGroup = new QGroupBox(i18nc("@title:group Decimal representation of coordinates", "Decimal"), page);
    QGridLayout *decimalLayout = new QGridLayout();
    decimalGroup->setLayout(decimalLayout);
    decimalLayout->setSpacing(spacingHint());

    QLabel *label = new QLabel(i18nc("@label:spinbox", "Latitude:"), decimalGroup);
    decimalLayout->addWidget(label, 0, 0);

    mLatitude = new QDoubleSpinBox(decimalGroup);
    mLatitude->setMinimum(-90);
    mLatitude->setMaximum(90);
    mLatitude->setSingleStep(1);
    mLatitude->setValue(0);
    mLatitude->setDecimals(6);
    mLatitude->setSuffix(QChar(176));
    decimalLayout->addWidget(mLatitude, 0, 1);

    label = new QLabel(i18nc("@label:spinbox", "Longitude:"), decimalGroup);
    decimalLayout->addWidget(label, 1, 0);

    mLongitude = new QDoubleSpinBox(decimalGroup);
    mLongitude->setMinimum(-180);
    mLongitude->setMaximum(180);
    mLongitude->setSingleStep(1);
    mLongitude->setValue(0);
    mLongitude->setDecimals(6);
    mLongitude->setSuffix(QChar(176));
    decimalLayout->addWidget(mLongitude, 1, 1);

    QGroupBox *sexagesimalGroup = new QGroupBox(i18nc("@title:group", "Sexagesimal"), page);
    QGridLayout *sexagesimalLayout = new QGridLayout();
    sexagesimalGroup->setLayout(sexagesimalLayout);
    sexagesimalLayout->setSpacing(spacingHint());

    label = new QLabel(i18nc("@label:spinbox", "Latitude:"), sexagesimalGroup);
    sexagesimalLayout->addWidget(label, 0, 0);

    mLatDegrees = new QSpinBox(sexagesimalGroup);
    mLatDegrees->setMinimum(0);
    mLatDegrees->setMaximum(90);
    mLatDegrees->setValue(1);
    mLatDegrees->setSuffix(QChar(176));
    mLatDegrees->setWrapping(false);
    label->setBuddy(mLatDegrees);
    sexagesimalLayout->addWidget(mLatDegrees, 0, 1);

    mLatMinutes = new QSpinBox(sexagesimalGroup);
    mLatMinutes->setMinimum(0);
    mLatMinutes->setMaximum(59);
    mLatMinutes->setValue(1);

    mLatMinutes->setSuffix(QStringLiteral("'"));
    sexagesimalLayout->addWidget(mLatMinutes, 0, 2);

    mLatSeconds = new QSpinBox(sexagesimalGroup);
    mLatSeconds->setMinimum(0);
    mLatSeconds->setMaximum(59);
    mLatSeconds->setValue(1);
    mLatSeconds->setSuffix(QStringLiteral("\""));
    sexagesimalLayout->addWidget(mLatSeconds, 0, 3);

    mLatDirection = new KComboBox(sexagesimalGroup);
    mLatDirection->addItem(i18nc("@item:inlistbox Latitude direction", "North"));
    mLatDirection->addItem(i18nc("@item:inlistbox Latitude direction", "South"));
    sexagesimalLayout->addWidget(mLatDirection, 0, 4);

    label = new QLabel(i18nc("@label:spinbox", "Longitude:"), sexagesimalGroup);
    sexagesimalLayout->addWidget(label, 1, 0);

    mLongDegrees = new QSpinBox(sexagesimalGroup);
    mLongDegrees->setMinimum(0);
    mLongDegrees->setMaximum(180);
    mLongDegrees->setValue(1);
    mLongDegrees->setSuffix(QChar(176));
    label->setBuddy(mLongDegrees);
    sexagesimalLayout->addWidget(mLongDegrees, 1, 1);

    mLongMinutes = new QSpinBox(sexagesimalGroup);
    mLongMinutes->setMinimum(0);
    mLongMinutes->setMaximum(59);
    mLongMinutes->setValue(1);
    mLongMinutes->setSuffix(QStringLiteral("'"));
    sexagesimalLayout->addWidget(mLongMinutes, 1, 2);

    mLongSeconds = new QSpinBox(sexagesimalGroup);
    mLongSeconds->setMinimum(0);
    mLongSeconds->setMaximum(59);
    mLongSeconds->setValue(1);
    mLongSeconds->setSuffix(QStringLiteral("\""));
    sexagesimalLayout->addWidget(mLongSeconds, 1, 3);

    mLongDirection = new KComboBox(sexagesimalGroup);
    mLongDirection->addItem(i18nc("@item:inlistbox Longtitude direction", "East"));
    mLongDirection->addItem(i18nc("@item:inlistbox Longtitude direction", "West"));
    sexagesimalLayout->addWidget(mLongDirection, 1, 4);

    layout->addWidget(decimalGroup);
    layout->addWidget(sexagesimalGroup);

    loadCityList();

    connect(mCityCombo, SIGNAL(activated(int)),
            SLOT(cityInputChanged()));
    connect(mLatitude, SIGNAL(valueChanged(double)),
            SLOT(decimalInputChanged()));
    connect(mLongitude, SIGNAL(valueChanged(double)),
            SLOT(decimalInputChanged()));
    connect(mLatDegrees, SIGNAL(valueChanged(int)),
            SLOT(sexagesimalInputChanged()));
    connect(mLatMinutes, SIGNAL(valueChanged(int)),
            SLOT(sexagesimalInputChanged()));
    connect(mLatSeconds, SIGNAL(valueChanged(int)),
            SLOT(sexagesimalInputChanged()));
    connect(mLatDirection, SIGNAL(activated(int)),
            SLOT(sexagesimalInputChanged()));
    connect(mLongDegrees, SIGNAL(valueChanged(int)),
            SLOT(sexagesimalInputChanged()));
    connect(mLongMinutes, SIGNAL(valueChanged(int)),
            SLOT(sexagesimalInputChanged()));
    connect(mLongSeconds, SIGNAL(valueChanged(int)),
            SLOT(sexagesimalInputChanged()));
    connect(mLongDirection, SIGNAL(activated(int)),
            SLOT(sexagesimalInputChanged()));

    updateInputs();
}

KABC::Geo GeoDialog::coordinates() const
{
    return mCoordinates;
}

void GeoDialog::cityInputChanged()
{
    if (mCityCombo->currentIndex() != 0) {
        GeoData geoData = mGeoDataMap[mCityCombo->currentText()];
        mCoordinates.setLatitude(geoData.latitude);
        mCoordinates.setLongitude(geoData.longitude);
    } else {
        mCoordinates.setLatitude(0);
        mCoordinates.setLongitude(0);
    }

    updateInputs(ExceptCity);
}

void GeoDialog::decimalInputChanged()
{
    mCoordinates.setLatitude(mLatitude->value());
    mCoordinates.setLongitude(mLongitude->value());

    updateInputs(ExceptDecimal);
}

void GeoDialog::sexagesimalInputChanged()
{
    double latitude = (double)(mLatDegrees->value() + (double)mLatMinutes->value() /
                               60 + (double)mLatSeconds->value() / 3600);
    latitude *= (mLatDirection->currentIndex() == 1 ? -1 : 1);

    double longitude = (double)(mLongDegrees->value() + (double)mLongMinutes->value() /
                                60 + (double)mLongSeconds->value() / 3600);
    longitude *= (mLongDirection->currentIndex() == 1 ? -1 : 1);

    mCoordinates.setLatitude(latitude);
    mCoordinates.setLongitude(longitude);

    updateInputs(ExceptSexagesimal);
}

void GeoDialog::updateInputs(ExceptType type)
{
    mCityCombo->blockSignals(true);
    mLatitude->blockSignals(true);
    mLongitude->blockSignals(true);
    mLatDegrees->blockSignals(true);
    mLatMinutes->blockSignals(true);
    mLatSeconds->blockSignals(true);
    mLatDirection->blockSignals(true);
    mLongDegrees->blockSignals(true);
    mLongMinutes->blockSignals(true);
    mLongSeconds->blockSignals(true);
    mLongDirection->blockSignals(true);

    if (!(type &ExceptDecimal)) {
        mLatitude->setValue(mCoordinates.latitude());
        mLongitude->setValue(mCoordinates.longitude());
    }

    if (!(type &ExceptSexagesimal)) {
        int degrees, minutes, seconds;
        double latitude = mCoordinates.latitude();
        double longitude = mCoordinates.longitude();

        latitude *= (latitude < 0 ? -1 : 1);
        longitude *= (longitude < 0 ? -1 : 1);

        degrees = (int)(latitude * 1);
        minutes = (int)((latitude - degrees) * 60);
        seconds = (int)((double)((double)latitude - (double)degrees - ((double)minutes / (double)60)) * (double)3600);

        mLatDegrees->setValue(degrees);
        mLatMinutes->setValue(minutes);
        mLatSeconds->setValue(seconds);

        mLatDirection->setCurrentIndex(mLatitude->value() < 0 ? 1 : 0);

        degrees = (int)(longitude * 1);
        minutes = (int)((longitude - degrees) * 60);
        seconds = (int)((double)(longitude - (double)degrees - ((double)minutes / 60)) * 3600);

        mLongDegrees->setValue(degrees);
        mLongMinutes->setValue(minutes);
        mLongSeconds->setValue(seconds);
        mLongDirection->setCurrentIndex(mLongitude->value() < 0 ? 1 : 0);
    }

    if (!(type &ExceptCity)) {
        const int index = nearestCity(mCoordinates.longitude(), mCoordinates.latitude());
        if (index != -1) {
            mCityCombo->setCurrentIndex(index + 1);
        } else {
            mCityCombo->setCurrentIndex(0);
        }
    }

    mCityCombo->blockSignals(false);
    mLatitude->blockSignals(false);
    mLongitude->blockSignals(false);
    mLatDegrees->blockSignals(false);
    mLatMinutes->blockSignals(false);
    mLatSeconds->blockSignals(false);
    mLatDirection->blockSignals(false);
    mLongDegrees->blockSignals(false);
    mLongMinutes->blockSignals(false);
    mLongSeconds->blockSignals(false);
    mLongDirection->blockSignals(false);
}

void GeoDialog::loadCityList()
{
    mCityCombo->clear();
    mGeoDataMap.clear();

    QFile file(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("akonadi/contact/data/zone.tab")));

    if (file.open(QIODevice::ReadOnly)) {
        QTextStream s(&file);

        QString line, country;
        QRegExp coord(QStringLiteral("[+-]\\d+[+-]\\d+"));
        QRegExp name(QStringLiteral("[^\\s]+/[^\\s]+"));
        int pos;

        while (!s.atEnd()) {
            line = s.readLine().trimmed();
            if (line.isEmpty() || line[0] == QLatin1Char('#')) {
                continue;
            }

            country = line.left(2);
            QString c, n;
            pos = coord.indexIn(line, 0);
            if (pos >= 0) {
                c = line.mid(pos, coord.matchedLength());
            }

            pos = name.indexIn(line, pos);
            if (pos > 0) {
                n = line.mid(pos, name.matchedLength()).trimmed();
            }

            if (!c.isEmpty() && !n.isEmpty()) {
                pos = c.indexOf(QLatin1Char('+'), 1);
                if (pos < 0) {
                    pos = c.indexOf(QLatin1Char('-'), 1);
                }
                if (pos > 0) {
                    GeoData geoData;
                    geoData.latitude = calculateCoordinate(c.left(pos));
                    geoData.longitude = calculateCoordinate(c.mid(pos));
                    geoData.country = country;

                    mGeoDataMap.insert(i18n(qPrintable(n)).replace(QLatin1Char('_'),  QLatin1Char(' ')), geoData);
                }
            }
        }

        QStringList items(mGeoDataMap.keys());
        items.prepend(i18nc("@item:inlistbox Undefined location", "Undefined"));
        mCityCombo->addItems(items);

        file.close();
    }
}

int GeoDialog::nearestCity(double x, double y) const
{
    QMap<QString, GeoData>::ConstIterator it;
    int pos = 0;
    for (it = mGeoDataMap.begin(); it != mGeoDataMap.end(); ++it, ++pos) {
        double dist = ((*it).longitude - x) * ((*it).longitude - x) +
                      ((*it).latitude - y) * ((*it).latitude - y);
        if (dist < 0.0005) {
            return pos;
        }
    }

    return -1;
}
