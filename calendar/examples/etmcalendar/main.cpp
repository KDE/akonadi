#include <Akonadi/Calendar/ETMCalendar>
#include <KCheckableProxyModel>

#include <QApplication>
#include <QTreeView>
#include <QHBoxLayout>

using namespace Akonadi;

int main( int argv, char **argc )
{
  QApplication app( argv, argc );

  ETMCalendar calendar;
  
  QWidget *window = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout( window );

  QTreeView *collectionSelectionView = new QTreeView();
  collectionSelectionView->setModel( calendar.checkableProxyModel() );

  QTreeView *itemView = new QTreeView();
  itemView->setModel( calendar.model() );

  layout->addWidget( collectionSelectionView );
  layout->addWidget( itemView );
  
  window->show();
  
  return app.exec();
}
