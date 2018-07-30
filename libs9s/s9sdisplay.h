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
#pragma once

#include "S9sString"
#include "S9sVariantMap"
#include "S9sEvent"
#include "S9sNode"

class S9sDisplay
{
    public:
        enum DisplayMode 
        {
            PrintEvents,
            WatchNodes,
        };

        S9sDisplay(S9sDisplay::DisplayMode mode = S9sDisplay::PrintEvents);
        virtual ~S9sDisplay();

        void eventCallback(S9sEvent &event);

        static void eventHandler(
                const S9sVariantMap &jsonMessage,
                void                *userData);

    private:
        void processEvent(S9sEvent &event);
        void processEventList(S9sEvent &event);
        void processEventWatchNodes(S9sEvent &event);

        void printHeader();
        void printNodes();

        char rotatingCharacter() const;

    private:
        DisplayMode         m_displayMode;
        int                 m_refreshCounter;
        S9sVector<S9sNode>  m_nodes;

};
