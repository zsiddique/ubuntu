/*
  Q Light Controller
  artnetout.h

  Copyright (c) Heikki Junnila
                Simon Newton

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  Version 2 as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details. The license is
  in the file "COPYING".

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef ARTNETOUT_H
#define ARTNETOUT_H

#include "artnetthread.h"

#include <qstring.h>
#include <qmutex.h>

#include <QDomDocument>

#include "qlcoutplugin.h"

#define CHANNELS 512

class ConfigureArtNetOutDialog;
class QPoint;

class ArtNetOut : public QLCOutPlugin
{
    Q_OBJECT
    Q_INTERFACES(QLCOutPlugin)

    friend class ConfigureArtNetOutDialog;

public:
    virtual ~ArtNetOut();

    // Plugin methods
    void init();
    QString name();

    void open(quint32 output = 0);
    void close(quint32 output = 0);
    bool isOpen();

    // configure
    int setConfigDirectory(QString dir);
    int saveSettings();
    int loadSettings();

    void configure();
    bool canConfigure() {
        return true;
    }

    // OutputPlugin methods
    QStringList outputs();
    void outputDMX(quint32 output, const QByteArray& universe);
    QString infoText(quint32 output = QLCOutPlugin::invalidOutput());

signals:
    void configurationChanged();

public slots:
    void newIp(QString ip);

private:
    void activate();
    void createContents(QDomDocument& domDoc);

private:
    QString m_configDir;			// config dir
    ArtNetThread *m_thr ;
    QString m_ip;
};

#endif
