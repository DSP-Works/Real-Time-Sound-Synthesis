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

#include <QDebug>
#include <QtVariantPropertyManager>
#include <QtVariantProperty>
#include <qmath.h>
#include "Application.h"
#include "ISignalChain.h"
#include "NoteOnEvent.h"
#include "NoteOffEvent.h"
#include "Envelope.h"

const QColor cDefaultColor(230, 240, 210);

void setDefaultAttrs(QtVariantProperty *pProp)
{
    pProp->setAttribute("decimals", 2);
    pProp->setAttribute("minimum", 0.0);
    pProp->setAttribute("singleStep", 0.01);
}

Envelope::Envelope(AudioUnitPlugin *pPlugin)
    : AudioUnit(pPlugin)
{
    m_pOutput = addOutput("gain");

    createProperties();
}

Envelope::~Envelope()
{
}

QColor Envelope::color() const
{
    return cDefaultColor;
}

void Envelope::serialize(QVariantMap &data, SerializationContext *pContext) const
{
    Q_ASSERT(pContext != nullptr);
    AudioUnit::serialize(data, pContext);
    data["attackTime"] = m_pAttackTimeMs->value();
    data["decayTime"] = m_pDecayTimeMs->value();
    data["sustainLevel"] = m_pSustainLevel->value();
    data["releaseTime"] = m_pReleaseTimeMs->value();
    data["signalChainEnable"] = m_pSignalChainEnable->value();
    data["signalChainDisable"] = m_pSignalChainDisable->value();
}

void Envelope::deserialize(const QVariantMap &data, SerializationContext *pContext)
{
    Q_ASSERT(pContext != nullptr);
    m_pAttackTimeMs->setValue(data["attackTime"]);
    m_pDecayTimeMs->setValue(data["decayTime"]);
    m_pSustainLevel->setValue(data["sustainLevel"]);
    m_pReleaseTimeMs->setValue(data["releaseTime"]);
    m_pSignalChainEnable->setValue(data["signalChainEnable"]);
    m_pSignalChainDisable->setValue(data["signalChainDisable"]);
    AudioUnit::deserialize(data, pContext);
}

void Envelope::processStart()
{
    m_state = State_Off;
    m_output = 0.0f;
    m_noteNumber = -1;

    m_attackTCO = exp(-1.5);
    m_decayTCO = exp(-4.95);
    m_releaseTCO = m_decayTCO;

    m_dt = signalChain()->timeStep();
    cachePropetties();

    calculateAttack();
    calculateDecay();
    calculateRelease();
}

void Envelope::processStop()
{
}

void Envelope::process()
{
    doEnvelope();

    // Recalculate
    //calculateAttack();
    //calculateDecay();
    //calculateRelease();

    m_pOutput->setValue(m_output);
}

void Envelope::reset()
{
}

void Envelope::doEnvelope()
{
    // Handle state switch
    switch (m_state) {
    case State_Off:
        m_output = 0.0;
        break;
    case State_Attack:
        m_output = m_attackOffset + m_output * m_attackCoeff;
        if (m_output >= 1.0 || m_attackTimeMs <= 0.0) {
            m_output = 1.0;
            setState(State_Decay);
        }
        break;
    case State_Decay:
        m_output = m_decayOffset + m_output * m_decayCoeff;
        if (m_output <= m_sustainLevel || m_decayTimeMs <= 0.0) {
            m_output = m_sustainLevel;
            if (m_sustainLevel >= 0.0f) {
                setState(State_Sustain);
            } else {
                // No need to sustain at zero level.
                setState(State_Off);
            }
        }
        break;
    case State_Sustain:
        m_output = m_sustainLevel;
        break;
    case State_Release:
        m_output = m_releaseOffset + m_output * m_releaseCoeff;
        if (m_output <= 0.0 || m_releaseTimeMs <= 0.0) {
            m_output = 0.0;
            setState(State_Off);
        }
        break;
    default:
        break;
    }
}

void Envelope::noteOnEvent(NoteOnEvent *pEvent)
{
    m_noteNumber = pEvent->noteNumber();
    // Previous note has no been released - do legato (keel sustain state).
    if (m_state != State_Sustain) {
        setState(State_Attack);
    }
}

void Envelope::noteOffEvent(NoteOffEvent *pEvent)
{
    if (m_noteNumber == pEvent->noteNumber()) {
        setState(State_Release);
    }
}

void Envelope::setState(State s)
{
    if (m_state != s) {
        m_state = s;

        switch (m_state) {
        case State_Attack:
            if (m_signalChainEnable) {
                signalChain()->enable(true);
            }
            break;
        case State_Off:
            if (m_signalChainDisable) {
                signalChain()->enable(false);
            }
            break;
        default:
            break;
        }
    }
}

void Envelope::calculateAttack()
{
    float samples = m_attackTimeMs / m_dt / 1000.0;
    m_attackCoeff = exp(-log((1.0 + m_attackTCO) / m_attackTCO) / samples);
    m_attackOffset = (1.0 + m_attackTCO) * (1.0 - m_attackCoeff);
}

void Envelope::calculateDecay()
{
    float samples = m_decayTimeMs / m_dt / 1000.0;
    m_decayCoeff = exp(-log((1.0 + m_decayTCO) / m_decayTCO) / samples);
    m_decayOffset = (m_sustainLevel - m_decayTCO) * (1.0 - m_decayCoeff);
}

void Envelope::calculateRelease()
{
    float samples = m_releaseTimeMs / m_dt / 1000.0;
    m_releaseCoeff = exp(-log((1.0 + m_releaseTCO) / m_releaseTCO) / samples);
    m_releaseOffset = -m_releaseTCO * (1.0 - m_releaseCoeff);
}

void Envelope::createProperties()
{
    QtVariantProperty *pRoot = rootProperty();

    // Create groups
    QtVariantProperty *pAttack = propertyManager()->addProperty(propertyManager()->groupTypeId(), "Attack");
    QtVariantProperty *pDecay = propertyManager()->addProperty(propertyManager()->groupTypeId(), "Decay");
    QtVariantProperty *pSustain = propertyManager()->addProperty(propertyManager()->groupTypeId(), "Sustain");
    QtVariantProperty *pRelease = propertyManager()->addProperty(propertyManager()->groupTypeId(), "Release");
    QtVariantProperty *pSignalChain = propertyManager()->addProperty(propertyManager()->groupTypeId(), "Signal chain");

    pRoot->addSubProperty(pAttack);
    pRoot->addSubProperty(pDecay);
    pRoot->addSubProperty(pSustain);
    pRoot->addSubProperty(pRelease);
    pRoot->addSubProperty(pSignalChain);

    // Attack
    m_pAttackTimeMs = propertyManager()->addProperty(QVariant::Double, "Time, ms");
    setDefaultAttrs(m_pAttackTimeMs);
    m_pAttackTimeMs->setValue(100.0);

    pAttack->addSubProperty(m_pAttackTimeMs);

    // Decay
    m_pDecayTimeMs = propertyManager()->addProperty(QVariant::Double, "Time, ms");
    setDefaultAttrs(m_pDecayTimeMs);
    m_pDecayTimeMs->setValue(1000.0);

    pDecay->addSubProperty(m_pDecayTimeMs);

    // Sustain
    m_pSustainLevel = propertyManager()->addProperty(QVariant::Double, "Level");
    setDefaultAttrs(m_pSustainLevel);
    m_pSustainLevel->setAttribute("maximum", 1.0);
    m_pSustainLevel->setValue(0.8);
    pSustain->addSubProperty(m_pSustainLevel);

    // Release
    m_pReleaseTimeMs = propertyManager()->addProperty(QVariant::Double, "Time, ms");
    setDefaultAttrs(m_pReleaseTimeMs);
    m_pReleaseTimeMs->setValue(300.0);
    pRelease->addSubProperty(m_pReleaseTimeMs);

    // Control signal chain
    m_pSignalChainEnable = propertyManager()->addProperty(QVariant::Bool, "Enable");
    m_pSignalChainEnable->setValue(false);
    m_pSignalChainEnable->setToolTip("Note on event enables signal chain");
    pSignalChain->addSubProperty(m_pSignalChainEnable);

    m_pSignalChainDisable = propertyManager()->addProperty(QVariant::Bool, "Disable");
    m_pSignalChainDisable->setValue(false);
    m_pSignalChainDisable->setToolTip("Release disables signal chain");
    pSignalChain->addSubProperty(m_pSignalChainDisable);

    // Properties change handler
    QObject::connect (propertyManager(), &QtVariantPropertyManager::propertyChanged, [this](QtProperty *pProperty){
        Q_UNUSED(pProperty);
        cachePropetties();
    });
}

void Envelope::cachePropetties()
{
    m_attackTimeMs = m_pAttackTimeMs->value().toFloat();
    m_decayTimeMs = m_pDecayTimeMs->value().toFloat();
    m_sustainLevel = m_pSustainLevel->value().toFloat();
    m_releaseTimeMs = m_pReleaseTimeMs->value().toFloat();
    m_signalChainEnable = m_pSignalChainEnable->value().toBool();
    m_signalChainDisable = m_pSignalChainDisable->value().toBool();

    calculateAttack();
    calculateDecay();
    calculateRelease();
}
