/*
  Q Light Controller
  genericfader.cpp

  Copyright (c) Heikki Junnila

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

#include <cmath>
#include <QDebug>

#include "universearray.h"
#include "genericfader.h"
#include "fadechannel.h"

GenericFader::GenericFader()
    : m_intensity(1)
{
}

GenericFader::~GenericFader()
{
}

void GenericFader::add(const FadeChannel& ch)
{
    if (m_channels.contains(ch.address()) == true)
    {
        if (m_channels[ch.address()].current() < ch.current())
            m_channels[ch.address()] = ch;
    }
    else
    {
        m_channels[ch.address()] = ch;
    }
}

void GenericFader::remove(quint32 address)
{
    if (m_channels.contains(address) == true)
        m_channels.remove(address);
}

void GenericFader::removeAll()
{
    m_channels.clear();
}

void GenericFader::write(UniverseArray* ua)
{
    QMutableHashIterator <quint32,FadeChannel> it(m_channels);
    while (it.hasNext() == true)
    {
        FadeChannel& fc(it.next().value());
        if (fc.elapsed() >= fc.fadeTime())
        {
            if (fc.group() == QLCChannel::Intensity || fc.isReady() == false)
            {
                fc.setReady(true);
                uchar value = uchar(floor((qreal(fc.target()) * intensity()) + 0.5));
                ua->write(fc.address(), value, fc.group());

                // Remove all channels that reach zero
                if (fc.target() == 0 && fc.current() == 0)
                    remove(fc.address());
            }
            else
            {
                // After an LTP channel becomes ready, its value is no longer written.
                // Remove it from the fader.
                remove(fc.address());
            }
        }
        else
        {
            uchar value = uchar(floor((qreal(fc.current()) * intensity()) + 0.5));
            ua->write(fc.address(), value, fc.group());
            fc.nextStep();
        }
    }
}

void GenericFader::adjustIntensity(qreal fraction)
{
    m_intensity = fraction;
}

qreal GenericFader::intensity() const
{
    return m_intensity;
}
