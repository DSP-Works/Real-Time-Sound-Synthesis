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

#include <QtVariantPropertyManager>
#include <QtVariantProperty>
#include <qmath.h>
#include "Application.h"
#include "DelayLine.h"
#include "ISignalChain.h"
#include "Delay.h"

Delay::Delay(AudioUnitPlugin *pPlugin)
    : AudioUnit(pPlugin)
{
    m_pInput = addInput("in");
    m_pDelayRatioInput = addInput("delay", 1.0f);
    m_pOutput = addOutput("out");

    m_pDelayLine = nullptr;

    createProperties();
}

Delay::~Delay()
{
    delete m_pDelayLine;
}

void Delay::serialize(QVariantMap &data, SerializationContext *pContext) const
{
    Q_ASSERT(pContext != nullptr);
    data["delay"] = m_pPropDelay->value();
    AudioUnit::serialize(data, pContext);
}

void Delay::deserialize(const QVariantMap &data, SerializationContext *pContext)
{
    Q_ASSERT(pContext != nullptr);
    m_pPropDelay->setValue(data["delay"]);
    AudioUnit::deserialize(data, pContext);
}

void Delay::processStart()
{
    float delayMs = m_pPropDelay->value().toFloat();
    m_delaySamples = delayMs / 1000.0 / signalChain()->timeStep() + 1;
    if (m_pDelayLine == nullptr || m_pDelayLine->samplesMax() != m_delaySamples) {
        // (re-)allocate delay line object
        delete m_pDelayLine;
        m_pDelayLine = new DelayLine(m_delaySamples);
    }
    m_pDelayLine->reset();
}

void Delay::processStop()
{
    // We do not delete delay line here to avoid
    // race condition when processing is still running.
}

void Delay::process()
{
    float delayRatio = qMin(1.0f, qMax(0.0f, m_pDelayRatioInput->getValue()));

    m_pDelayLine->setDelayFraction(delayRatio);

    float out = m_pDelayLine->process(m_pInput->getValue());
    m_pOutput->setValue(out);
}

void Delay::reset()
{
    if (m_pDelayLine != nullptr) {
        m_pDelayLine->reset();
    }
}

void Delay::createProperties()
{
    QtVariantProperty *pRoot = rootProperty();

    m_pPropDelay = propertyManager()->addProperty(QVariant::Double, "Maximum delay, ms");
    m_pPropDelay->setValue(100.0);
    m_pPropDelay->setAttribute("minimum", 0.0);
    m_pPropDelay->setAttribute("maximum", 5000.0);
    m_pPropDelay->setAttribute("decimals", 2);
    m_pPropDelay->setAttribute("singleStep", 1.0);
    pRoot->addSubProperty(m_pPropDelay);
}
