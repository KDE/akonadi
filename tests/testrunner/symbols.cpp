/*
 * Copyright (c) 2008  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "symbols.h"

Symbols* Symbols::instance = 0;

Symbols *Symbols::getInstance()
{
  if ( instance == 0 ) {
    instance= new Symbols();
  }

  return instance;
}

void Symbols::destroyInstance()
{
  delete instance;
}

void Symbols::insertSymbol( const QString &key, const QString &item )
{
  symbols.insert( key, item );
}

QHash<QString, QString> Symbols::getSymbols() const
{
  return symbols;
}

