/*****************************************************************************
 * Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                      *
 *                                                                           *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Library General Public               *
 * License version 2 as published by the Free Software Foundation.           *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public License *
 * along with this library; see the file COPYING.LIB.  If not, write to      *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 * Boston, MA 02110-1301, USA.                                               *
 *****************************************************************************/

#include "kedittagsdialog_p.h"

#include <kicon.h>
#include <klineedit.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kcheckableproxymodel.h>

#include <akonadi/changerecorder.h>
#include <akonadi/tagcreatejob.h>
#include <akonadi/tagdeletejob.h>
#include <akonadi/tagfetchscope.h>
#include <akonadi/tagattribute.h>

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

KEditTagsDialog::KEditTagsDialog(const Akonadi::Tag::List& tags,
                                 Akonadi::TagModel *model,
                                 QWidget* parent) :
    KDialog(parent),
    m_tags(tags),
    m_model(model),
    m_tagsView(0),
    m_newTagButton(0),
    m_newTagEdit(0),
    m_deleteButtonTimer(0)
{

    const QString caption = ( tags.count() > 0 ) ?
                            i18nc( "@title:window", "Change Tags" ) :
                            i18nc( "@title:window", "Add Tags" );
    setCaption( caption );
    setButtons( KDialog::Ok | KDialog::Cancel );
    setDefaultButton( KDialog::Ok );

    QWidget *mainWidget = new QWidget( this );
    QVBoxLayout *topLayout = new QVBoxLayout( mainWidget );

    QLabel *label = new QLabel( i18nc( "@label:textbox",
                                       "Configure which tags should "
                                       "be applied." ), this );

    QItemSelectionModel *selectionModel = new QItemSelectionModel( m_model, this );
    m_checkableProxy = new KCheckableProxyModel( this );
    m_checkableProxy->setSourceModel( m_model );
    m_checkableProxy->setSelectionModel( selectionModel );

    m_tagsView = new QListView( this );
    m_tagsView->setMouseTracking( true );
    m_tagsView->setSelectionMode( QAbstractItemView::NoSelection );
    m_tagsView->installEventFilter( this );
    m_tagsView->setModel( m_checkableProxy );
    connect( m_tagsView, SIGNAL(entered(QModelIndex)),
             this, SLOT(slotItemEntered(QModelIndex)) );

    m_newTagEdit = new KLineEdit( this );
    m_newTagEdit->setClearButtonShown( true );
    connect(m_newTagEdit, SIGNAL(textEdited(QString)),
            this, SLOT(slotTextEdited(QString)) );

    m_newTagButton = new QPushButton( i18nc( "@label", "Create new tag" ) );
    m_newTagButton->setEnabled( false );
    connect( m_newTagButton , SIGNAL(clicked(bool)),
             this, SLOT(slotCreateTag()) );

    QHBoxLayout *newTagLayout = new QHBoxLayout();
    newTagLayout->addWidget( m_newTagEdit, 1 );
    newTagLayout->addWidget( m_newTagButton );

    topLayout->addWidget( label );
    topLayout->addWidget( m_tagsView );
    topLayout->addLayout( newTagLayout );

    setMainWidget( mainWidget );

    // create the delete button, which is shown when
    // hovering the items
    m_deleteButton = new QPushButton( m_tagsView->viewport() );
    m_deleteButton->setIcon( KIcon( QLatin1String( "edit-delete" ) ) );
    m_deleteButton->setToolTip( i18nc( "@info", "Delete tag" ) );
    m_deleteButton->hide();
    connect( m_deleteButton, SIGNAL(clicked()), this, SLOT(deleteTag()) );

    m_deleteButtonTimer = new QTimer( this );
    m_deleteButtonTimer->setSingleShot( true );
    m_deleteButtonTimer->setInterval( 500 );
    connect( m_deleteButtonTimer, SIGNAL(timeout()), this, SLOT(showDeleteButton()) );


    // Check selected tags
    QItemSelection selection;
    for (int i = 0; i < m_model->rowCount(); ++i) {
      const QModelIndex index = m_model->index(i, 0 );
      const Akonadi::Tag insertedTag = index.data(Akonadi::TagModel::TagRole).value<Akonadi::Tag>();
      if (m_tags.contains(insertedTag)) {
        selection.select(index, index);
      }
    }
    m_checkableProxy->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);

    readConfig();
}

KEditTagsDialog::~KEditTagsDialog()
{
    writeConfig();
}

void KEditTagsDialog::readConfig()
{
    KConfigGroup group( KGlobal::config(), "KEditTagsDialog" );
    const QSize sizeDialog = group.readEntry( "Size", QSize(500,400) );
    if ( sizeDialog.isValid() ) {
        resize( sizeDialog );
    }
}

void KEditTagsDialog::writeConfig()
{
    KConfigGroup group( KGlobal::config(), "KEditTagsDialog" );
    group.writeEntry( "Size", size() );
}

Akonadi::Tag::List KEditTagsDialog::tags() const
{
    return m_tags;
}

bool KEditTagsDialog::eventFilter(QObject* watched, QEvent* event)
{
    if ( ( watched == m_tagsView ) && ( event->type() == QEvent::Leave ) ) {
        m_deleteButtonTimer->stop();
        m_deleteButton->hide();
    }
    return KDialog::eventFilter( watched, event );
}

void KEditTagsDialog::slotButtonClicked(int button)
{
    if ( button == KDialog::Ok ) {
        // update m_tags with the checked values, so
        // that the caller of the KEditTagsDialog can
        // receive the tags by KEditTagsDialog::tags()
        m_tags.clear();

        const int count = m_checkableProxy->rowCount();
        for ( int i = 0; i < count; ++i ) {
            if ( m_checkableProxy->selectionModel()->isRowSelected( i, QModelIndex() ) ) {
                const QModelIndex index = m_checkableProxy->index( i, 0, QModelIndex() );
                const Akonadi::Tag tag = index.data( Akonadi::TagModel::TagRole ).value<Akonadi::Tag>();
                m_tags.append( tag );
            }
        }

        accept();
    } else {
        KDialog::slotButtonClicked( button );
    }
}

void KEditTagsDialog::slotCreateTag()
{
    Akonadi::TagCreateJob *createJob = new Akonadi::TagCreateJob( Akonadi::Tag( m_newTagEdit->text() ), this );
    connect( createJob, SIGNAL(finished(KJob*)),
             this, SLOT(slotCreateTagFinished(KJob*)) );

    m_newTagEdit->clear();
    m_newTagEdit->setEnabled( false );
    m_newTagButton->setEnabled( false );
}

void KEditTagsDialog::slotCreateTagFinished( KJob *job )
{
    if ( job->error() ) {
        KMessageBox::error( this, i18n( "An error occurred while creating a new tag" ),
                            i18n( "Failed to create a new tag" ) );
    }

    m_newTagEdit->setEnabled( true );
}

void KEditTagsDialog::slotTextEdited(const QString &text)
{
    // Remove unnecessary spaces from a new tag is
    // mandatory, as the user cannot see the difference
    // between a tag "Test" and "Test ".
    const QString tagText = text.simplified();
    if ( tagText.isEmpty() ) {
        m_newTagButton->setEnabled( false );
        return;
    }

    // Check whether the new tag already exists
    const int count = m_checkableProxy->rowCount();
    bool exists = false;
    for ( int i = 0; i < count; ++i ) {
        const QModelIndex index = m_checkableProxy->index( i, 0, QModelIndex() );
        if ( index.data( Qt::DisplayRole ).toString() == tagText ) {
            exists = true;
            break;
        }
    }
    m_newTagButton->setEnabled( !exists );
}

void KEditTagsDialog::slotItemEntered( const QModelIndex &index )
{
    // align the delete-button to stay on the right border
    // of the item
    const QRect rect = m_tagsView->visualRect( index );
    const int size = rect.height();
    const int x = rect.right() - size;
    const int y = rect.top();
    m_deleteButton->move( x, y );
    m_deleteButton->resize( size, size );

    m_deleteCandidate = index;
    m_deleteButtonTimer->start();
}

void KEditTagsDialog::showDeleteButton()
{
    m_deleteButton->show();
}

void KEditTagsDialog::deleteTag()
{
    Q_ASSERT( m_deleteCandidate.isValid() );
    const Akonadi::Tag tag = m_deleteCandidate.data( Akonadi::TagModel::TagRole ).value<Akonadi::Tag>();
    const QString text = i18nc( "@info",
                                "Should the tag <resource>%1</resource> really be deleted for all files?",
                                tag.name() );
    const QString caption = i18nc( "@title", "Delete tag" );
    const KGuiItem deleteItem( i18nc( "@action:button", "Delete" ), KIcon( QLatin1String( "edit-delete" ) ) );
    const KGuiItem cancelItem( i18nc( "@action:button", "Cancel" ), KIcon( QLatin1String( "dialog-cancel" ) ) );
    if ( KMessageBox::warningYesNo( this, text, caption, deleteItem, cancelItem ) == KMessageBox::Yes ) {
        new Akonadi::TagDeleteJob( tag, this  );
    }
}

#include "moc_kedittagsdialog_p.cpp"
