#include "symbols.h"
#include <QtTest>

Symbols* Symbols::instance = 0;

Symbols *Symbols::getInstance()
{
  if(instance == 0){

    instance= new Symbols();
  }
  return instance;
}

void Symbols::destroyInstance()
{
  delete instance;
}

void Symbols::insertSymbol(QString key, QString item)
{
  symbols.insert(key, item);
}

QHash<QString, QString> Symbols::getSymbols()
{
  return symbols;
}

