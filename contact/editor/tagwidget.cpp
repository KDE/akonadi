/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "tagwidget.h"

#include <akonadi/tagmodel.h>
#include <akonadi/changerecorder.h>

#include "kedittagsdialog_p.h"

#include <kicon.h>
#include <klocalizedstring.h>
#include <ksqueezedtextlabel.h>

#include <QHBoxLayout>
#include <QToolButton>

TagWidget::TagWidget( QWidget *parent )
  : QWidget( parent )
{
  Akonadi::ChangeRecorder *recorder = new Akonadi::ChangeRecorder( this );
  recorder->setChangeRecordingEnabled( false );
  recorder->setTypeMonitored( Akonadi::Monitor::Tags );

  mModel = new Akonadi::TagModel( recorder, this );

  QHBoxLayout *layout = new QHBoxLayout( this );
  mTagLabel = new KSqueezedTextLabel;
  mTagLabel->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  layout->addWidget( mTagLabel );

  QToolButton *editButton = new QToolButton;
  editButton->setText( i18n( "..." ) );
  layout->addWidget( editButton, Qt::AlignRight );

  layout->setStretch( 0, 10 );

  connect( editButton, SIGNAL(clicked()), SLOT(editTags()) );
}

TagWidget::~TagWidget()
{
}

void TagWidget::setTags( const Akonadi::Tag::List &tags )
{
  mTags = tags;
  updateView();
}

Akonadi::Tag::List TagWidget::tags() const
{
  return mTags;
}

void TagWidget::editTags()
{
  QScopedPointer<KEditTagsDialog> dlg( new KEditTagsDialog( mTags, mModel, this ) );
  if ( dlg->exec() ) {
    mTags = dlg->tags();
    updateView();
  }
}

void TagWidget::updateView()
{
  QStringList tagsNames;
  // Load the real tag names from the model
  for (int i = 0; i < mModel->rowCount(); ++i) {
    const QModelIndex index = mModel->index( i, 0 );
    const Akonadi::Tag tag = mModel->data( index, Akonadi::TagModel::TagRole ).value<Akonadi::Tag>();
    if ( mTags.contains( tag ) ) {
      tagsNames << tag.name();
    }
  }

  mTagLabel->setText( tagsNames.join( QLatin1String( ", " ) ) );
}

#include "tagwidget.h"
