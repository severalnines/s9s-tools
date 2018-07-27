/*
 * Severalnines Tools
 * Copyright (C) 2016-2018 Severalnines AB
 *
 * This file is part of s9s-tools.
 *
 * s9s-tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * s9s-tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with s9s-tools. If not, see <http://www.gnu.org/licenses/>.
 */
#include "s9sdisplay.h"

#include "S9sOptions"
#include "S9sCluster"

//#define DEBUG
//#define WARNING
#include "s9sdebug.h"

S9sDisplay::S9sDisplay()
{
}

S9sDisplay::~S9sDisplay()
{
}

void 
S9sDisplay::eventCallback(
        S9sEvent &event)
{
    S9sOptions       *options = S9sOptions::instance();

    // Filtration by event class and subclass (event-name).
    if (!options->eventTypeEnabled(event.eventTypeString()))
        return;
    
    if (!options->eventNameEnabled(event.eventName()))
        return;

    // optional filtration by clusterId
    if (options->clusterId() > S9S_INVALID_CLUSTER_ID
        && options->clusterId() != event.clusterId())
        return;

    processEvent(event);
}


void 
S9sDisplay::processEvent(
        S9sEvent &event)
{
    processEventList(event);
}

void 
S9sDisplay::processEventList(
        S9sEvent &event)
{
    S9sOptions       *options = S9sOptions::instance();
    S9sString         output;

    if (options->isJsonRequested())
    {
        output = event.toVariantMap().toString();
    } else {
        output = event.toOneLiner();
    }

    if (!output.empty())
        ::printf("%s\n", STR(output));
}

void
S9sDisplay::eventHandler(
        const S9sVariantMap &jsonMessage,
        void                *userData)
{
    if (userData == NULL)
    {
        S9S_WARNING("userData is NULL.");
        return;
    }

    if (!jsonMessage.contains("class_name") ||
            jsonMessage.at("class_name").toString() != "CmonEvent")
    {
        return;
    }

    S9sEvent event = jsonMessage;
    S9sDisplay *display = (S9sDisplay *) userData;

    display->eventCallback(event);
}
