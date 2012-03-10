/*
  Q Light Controller
  artnetthread.h

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

#ifndef ARTNETTHREAD_H
#define ARTNETTHREAD_H

#include <qthread.h>
#include <qstring.h>
#include <qstringlist.h>
#include <artnet/artnet.h>
#include <sys/types.h>

class QTimer;

class ArtNetThread : public QThread
{
    Q_OBJECT

public:
    ArtNetThread(QString ip = "");
    ~ArtNetThread();

    // methods
    virtual void run();
    void start ( Priority priority = InheritPriority) ;
    void stop() ;
    void setIp(QString ip);
    int write_dmx(uint8_t *data , int channels) ;
    /** get ips, where artnet is sending the packets **/
    QStringList getNodeIps();

public slots:
    void searchDevices();
private:
    int startNode();
    int stopNode();
    int m_sd[2] ;
    artnet_node m_node;
    QString m_ip;
    QString m_newgw;
    bool m_configChanged;
    QByteArray m_oldDmxData; /* save bandwidth - only send artnet if dmxdata changed */
    QTimer* m_timer;

};

#endif
