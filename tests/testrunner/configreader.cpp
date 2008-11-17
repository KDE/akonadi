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

#include "configreader.h"
#include "config.h"
#include "symbols.h"

#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QtTest>

ConfigReader::ConfigReader(const QString &configfile) {
  QDomDocument doc;
  QFile file( configfile );

  if( !file.open( QIODevice::ReadOnly ) ){
    qDebug()<<"error reading file:"<<configfile;
    return;
  }else{
    if( !doc.setContent( &file ) ) {
      file.close();
      return;
    }
  }
  file.close();

  QDomElement root = doc.documentElement();
  if( root.tagName() != "config" )
    qDebug()<<"could not file root tag";

  QDomNode n = root.firstChild();
  while( !n.isNull() ) {
    QDomElement e = n.toElement();
    if( !e.isNull() ) {
      if( e.tagName() == "kdehome" ){
        setKdeHome( e.text() );
      } else {
        if( e.tagName() == "confighome" ){
          setXdgConfigHome( e.text() );
        } else {
          if( e.tagName() == "datahome" ){
            setXdgDataHome( e.text() );
          } else {
            if( e.tagName() == "item"){
              insertItemConfig( e.attribute("location"), e.attribute("collection") );
            } else {
              if( e.tagName() == "agent"){
                insertAgent( e.text() );
              }
            }
          }
        }
      }
    }

    n = n.nextSibling();
  }
}
