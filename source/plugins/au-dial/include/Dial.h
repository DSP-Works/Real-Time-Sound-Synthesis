/*
                          qmusic

    Copyright (C) 2015 Arthur Benilov,
    arthur.benilov@gmail.com

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.
*/

#ifndef AU_DIAL_H
#define AU_DIAL_H

#include "AudioUnit.h"

class QtVariantProperty;
class QDial;

/**
 * Dial control that outputs a single values
 * determined by the dial position set.
 */
class Dial : public AudioUnit
{
public:

    Dial(AudioUnitPlugin *pPlugin);
    ~Dial();

protected:

    void processStart();
    void processStop();
    void process();
    void reset();

    QGraphicsItem* graphicsItem();
    int flags() const;

    QString title() const;

    // ISerializable interface
    void serialize(QVariantMap &data, SerializationContext *pContext) const;
    void deserialize(const QVariantMap &data, SerializationContext *pContext);

private:

    void createProperties();
    void updateDialValues();
    void onDialValueChanged(int pos);

    OutputPort *m_pOutput;
    QtVariantProperty *m_pPropLabel;
    QtVariantProperty *m_pPropValue;
    QtVariantProperty *m_pPropMin;
    QtVariantProperty *m_pPropMax;
    QtVariantProperty *m_pPropSteps;

    QDial *m_pDial;
};

#endif // AU_DIAL_H

