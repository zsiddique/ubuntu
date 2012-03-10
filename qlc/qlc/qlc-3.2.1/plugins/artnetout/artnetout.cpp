/*
  Q Light Controller
  artnetout.cpp

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

#include "artnetout.h"
#include "artnetthread.h"

#include "configureartnetoutdialog.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <qthread.h>
#include <qstring.h>
#include <qpoint.h>
#include <qfile.h>
#include <qdir.h>
#include <errno.h>
#include <assert.h>

#include <qtextstream.h>
#include <../../engine/src/qlcfile.h>

#define CONF_FILE "artnetout.conf"

#define ID_CONFIGURE      10
#define ID_ACTIVATE       20

static QMutex _mutex;

//
// Class implementation
//
void ArtNetOut::init()
{
    m_thr = NULL;
    m_configDir = QString(QDir::homePath()).append("/.qlc/");
    m_ip = "";
    loadSettings();
}

ArtNetOut::~ArtNetOut()
{

}

QString ArtNetOut::name()
{
    return QString("ArtNet Output");
}

/* start the thread */
void ArtNetOut::open(quint32 output)
{
    Q_UNUSED(output);
    printf("open\n") ;
    // check this for error
    m_thr = new ArtNetThread(m_ip) ;
    m_thr->start() ;
}

/* terminate the thread */
void ArtNetOut::close(quint32 output)
{
    Q_UNUSED(output);
    printf("close\n") ;
    if(m_thr == NULL) {
        return ;
    }

    if(m_thr->isRunning()) {
        m_thr->stop() ;
    }
    // wait for thread termination
    m_thr->wait() ;

    delete m_thr ;
    m_thr = NULL ;
    return ;
}

/* check if thread is running */
bool ArtNetOut::isOpen()
{
    if(m_thr == NULL) {
        return false ;
    }

    return m_thr->isRunning() ;
}

/* called on configure */
void ArtNetOut::configure()
{
    ConfigureArtNetOutDialog* conf = new ConfigureArtNetOutDialog(this);

    if (conf->exec() == QDialog::Accepted) {
        m_ip = conf->ip();
        saveSettings();
    }

    delete conf;
}


/* return info string (allows html) */
QString ArtNetOut::infoText(quint32 output)
{
    if (output == QLCOutPlugin::invalidOutput()) {
        // plugin info
        QString t;
        QString str = QString::null;
        str += QString("<HTML><HEAD><TITLE>Plugin Info</TITLE></HEAD><BODY>");
        str += QString("<TABLE COLS=\"1\" WIDTH=\"100%\"><TR>");
        str += QString("<TD BGCOLOR=\"black\"><FONT COLOR=\"white\" SIZE=\"5\">");
        str += name() + QString("</FONT></TD></TR></TABLE>");
        str += QString("<TABLE COLS=\"2\" WIDTH=\"100%\">");
        str += QString("<TR>\n");
        str += QString("<TD><B>Version</B></TD>");
        str += QString("<TD>");
        str += QString("</TD>");
        str += QString("</TR>");

        str += QString("<TR>\n");
        str += QString("<TD><B>Status</B></TD>");
        str += QString("<TD>");
        if (isOpen() == true) {
            str += QString("<I>Active</I></TD>");
        } else {
            str += QString("Not Active</TD>");
        }
        str += QString("</TR>");

        str += QString("</TR>");
        str += QString("</TABLE>");
        str += QString("</BODY></HTML>");
        return str;
    }
    // output info
    if(m_thr == NULL) {
        return QString("Node not Running");
    } else {
        m_thr->getNodeIps();
    }

    return QString("artnet output is running");
}

/* set config directory */
int ArtNetOut::setConfigDirectory(QString dir)
{
    m_configDir = dir;
    return 0;
}

/* save settings to config file */
int ArtNetOut::saveSettings()
{
    QString fileName = m_configDir + QString(CONF_FILE);
    qDebug("%s", fileName.toAscii().data());
    QFile file(fileName);

    if (file.open(QFile::WriteOnly)) {
        QDomDocument dom("ArtNetConfig");

        QDomElement root = dom.createElement("ArtNetConfig");
        dom.appendChild(root);

        QDomElement bindIp = dom.createElement("BindIp");
        bindIp.appendChild(dom.createTextNode(m_ip));
        root.appendChild(bindIp);

        QString document = dom.toString();

        file.write(dom.toByteArray());
        file.close();
    } else {
        perror("file.open");
        qDebug("Unable to save ArtNetOut configuration");
    }
    return errno;
}


/* load settings from config file */
int ArtNetOut::loadSettings()
{
    QString fileName = m_configDir + QString(CONF_FILE);

    QDomDocument dom = QLCFile::readXML(fileName);
    createContents(dom);

    return 0;
}

/* */
void ArtNetOut::createContents(QDomDocument& domDoc)
{
    QDomElement root = domDoc.firstChildElement("ArtNetConfig");
    if(root.isNull()) {
        printf("could not read configfile");
        return;
    }
    QDomNode node = root.firstChild();
    QDomElement property;
    while(!node.isNull()) {
        property = node.toElement();
        if(property.tagName() == "BindIp") {
            m_ip = property.text();
        }
        node = node.nextSibling();
    }
}

/* start  */
void ArtNetOut::activate()
{
    emit configurationChanged();
}

QStringList ArtNetOut::outputs()
{
    QStringList outputs;
    outputs << QString("artnet");
    return outputs;
}

/* Write an universe out to artnet
 *
 */
void ArtNetOut::outputDMX(quint32 output, const QByteArray& universe)
{
    Q_UNUSED(output);
    int r ;

    _mutex.lock();

    if(m_thr == NULL) {
        return ;
    }

    // write to pipe
    r = m_thr->write_dmx((uint8_t *) universe.constData(), universe.size() ) ;

    _mutex.unlock();

    return ;
}

void ArtNetOut::newIp(QString ip)
{
    m_ip = ip;
    if (m_thr != NULL) {
        m_thr->setIp(ip);
    }
}


Q_EXPORT_PLUGIN2(artnetout, ArtNetOut)

