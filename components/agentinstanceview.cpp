/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#include <QtGui/QApplication>
#include <QtGui/QHBoxLayout>
#include <QtGui/QListView>
#include <QtGui/QPainter>

#include <libakonadi/agentinstancemodel.h>

#include "agentinstanceview.h"

using namespace PIM;

class AgentInstanceViewDelegate : public QAbstractItemDelegate
{
  public:
    AgentInstanceViewDelegate( QObject *parent = 0 );

    virtual void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;
    virtual QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const;

  private:
    void drawFocus( QPainter*, const QStyleOptionViewItem&, const QRect& ) const;
};

class AgentInstanceView::Private
{
  public:
    Private( AgentInstanceView *parent )
      : mParent( parent )
    {
    }

    void currentAgentInstanceChanged( const QModelIndex&, const QModelIndex& );

    AgentInstanceView *mParent;
    QListView *mView;
    AgentInstanceModel *mModel;
};

void AgentInstanceView::Private::currentAgentInstanceChanged( const QModelIndex &currentIndex, const QModelIndex &previousIndex )
{
  QString currentIdentifier;
  if ( currentIndex.isValid() )
    currentIdentifier = currentIndex.model()->data( currentIndex, Qt::UserRole ).toString();

  QString previousIdentifier;
  if ( previousIndex.isValid() )
    previousIdentifier = previousIndex.model()->data( previousIndex, Qt::UserRole ).toString();

  emit mParent->currentChanged( currentIdentifier, previousIdentifier );
}

AgentInstanceView::AgentInstanceView( QWidget *parent )
  : QWidget( parent ), d( new Private( this ) )
{
  QHBoxLayout *layout = new QHBoxLayout( this );
  layout->setMargin( 0 );
  layout->setSpacing( 0 );

  d->mView = new QListView( this );
  d->mView->setItemDelegate( new AgentInstanceViewDelegate( d->mView ) );
  layout->addWidget( d->mView );

  d->mModel = new AgentInstanceModel( d->mView );
  d->mView->setModel( d->mModel );

  d->mView->selectionModel()->setCurrentIndex( d->mModel->index( 0, 0 ), QItemSelectionModel::Select );
  d->mView->scrollTo( d->mModel->index( 0, 0 ) );
  connect( d->mView->selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ),
           this, SLOT( currentAgentInstanceChanged( const QModelIndex&, const QModelIndex& ) ) );
}

AgentInstanceView::~AgentInstanceView()
{
  delete d;
}

QString AgentInstanceView::currentAgentInstance() const
{
  QItemSelectionModel *selectionModel = d->mView->selectionModel();
  if ( !selectionModel )
    return QString();

  QModelIndex index = selectionModel->currentIndex();
  if ( !index.isValid() )
    return QString();

  return index.model()->data( index, Qt::UserRole ).toString();
}

AgentInstanceViewDelegate::AgentInstanceViewDelegate( QObject *parent )
 : QAbstractItemDelegate( parent )
{
}

void AgentInstanceViewDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  if ( !index.isValid() )
    return;

  painter->setRenderHint( QPainter::Antialiasing );

  const QString name = index.model()->data( index, Qt::DisplayRole ).toString();
  const QString status = index.model()->data( index, AgentInstanceModel::StatusMessageRole ).toString();

  const QVariant data = index.model()->data( index, Qt::DecorationRole );

  QPixmap pixmap;
  if ( data.isValid() && data.type() == QVariant::Icon )
    pixmap = qvariant_cast<QIcon>( data ).pixmap( 64, 64 );

  const QFont oldFont = painter->font();
  QFont boldFont( oldFont );
  boldFont.setBold( true );
  painter->setFont( boldFont );
  QFontMetrics fm = painter->fontMetrics();
  int hn = fm.boundingRect( 0, 0, 0, 0, Qt::AlignLeft, name ).height();
  int wn = fm.boundingRect( 0, 0, 0, 0, Qt::AlignLeft, name ).width();
  painter->setFont( oldFont );

  fm = painter->fontMetrics();
  int hc = fm.boundingRect( 0, 0, 0, 0, Qt::AlignLeft, status ).height();
  int wc = fm.boundingRect( 0, 0, 0, 0, Qt::AlignLeft, status ).width();
  int wp = pixmap.width();

  QPen pen = painter->pen();
  QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                            ? QPalette::Normal : QPalette::Disabled;
  if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
    cg = QPalette::Inactive;
  if (option.state & QStyle::State_Selected) {
    painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
    painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
  } else {
    painter->setPen(option.palette.color(cg, QPalette::Text));
  }

  QFont font = painter->font();
  painter->setFont(option.font);

  painter->drawPixmap( option.rect.x() + 5, option.rect.y() + 5, pixmap );

  painter->setFont(boldFont);
  if ( !name.isEmpty() )
    painter->drawText( option.rect.x() + 5 + wp + 5, option.rect.y() + 7, wn, hn, Qt::AlignLeft, name );
  painter->setFont(oldFont);

  if ( !status.isEmpty() )
    painter->drawText( option.rect.x() + 5 + wp + 5, option.rect.y() + 7 + hn, wc, hc, Qt::AlignLeft, status );

  painter->setPen(pen);

  drawFocus( painter, option, option.rect );
}

QSize AgentInstanceViewDelegate::sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  if ( !index.isValid() )
    return QSize( 0, 0 );

  const QString name = index.model()->data( index, Qt::DisplayRole ).toString();
  const QString status = index.model()->data( index, AgentInstanceModel::StatusMessageRole ).toString();

  QFontMetrics fm = option.fontMetrics;
  int hn = fm.boundingRect( 0, 0, 0, 0, Qt::AlignLeft, name ).height();
  int wn = fm.boundingRect( 0, 0, 0, 0, Qt::AlignLeft, name ).width();
  int hc = fm.boundingRect( 0, 0, 0, 0, Qt::AlignLeft, status ).height();
  int wc = fm.boundingRect( 0, 0, 0, 0, Qt::AlignLeft, status ).width();

  int width = 0;
  int height = 0;

  if ( !name.isEmpty() ) {
    height += hn;
    width = qMax( width, wn );
  }

  if ( !status.isEmpty() ) {
    height += hc;
    width = qMax( width, wc );
  }

  height = qMax( height, 64 ) + 10;
  width += 64 + 15;

  return QSize( width, height );
}

void AgentInstanceViewDelegate::drawFocus( QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect ) const
{
  if (option.state & QStyle::State_HasFocus) {
    QStyleOptionFocusRect o;
    o.QStyleOption::operator=(option);
    o.rect = rect;
    o.state |= QStyle::State_KeyboardFocusChange;
    QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled)
                              ? QPalette::Normal : QPalette::Disabled;
    o.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected)
                                             ? QPalette::Highlight : QPalette::Background);
    QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);
  }
}

#include "agentinstanceview.moc"
