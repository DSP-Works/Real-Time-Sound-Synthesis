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

#ifndef PITCHBENDEVENT_H
#define PITCHBENDEVENT_H

#include "FrameworkApi.h"
#include "SignalChainEvent.h"

class QMUSIC_FRAMEWORK_API PitchBendEvent : public SignalChainEvent
{
public:
    PitchBendEvent(int bend = 0);
    PitchBendEvent(const PitchBendEvent &evt);
    PitchBendEvent& operator =(const PitchBendEvent &evt);
    SignalChainEvent* clone() const;

    QString toString() const;

    int bend() const { return m_bend; }
    float bendFactor(int semitones = 1);

private:

    int m_bend; ///< Pitch bending value
};

#endif // PITCHBENDEVENT_H

