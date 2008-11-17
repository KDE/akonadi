#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <QHash>

class Symbols
{
  private:
    QHash<QString,QString> symbols;
    static Symbols *instance;    

  public:
    static Symbols *getInstance();
    static void destroyInstance();
    QHash<QString, QString> getSymbols();
    void insertSymbol(QString key, QString item);
};    

#endif
