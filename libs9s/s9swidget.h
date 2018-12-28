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
#include "S9sVariant"
#include "S9sVariantMap"

class S9sWidget
{
    public:
        S9sWidget();
        virtual ~S9sWidget();

        virtual void setLocation(int x, int y);
        virtual void setSize(int nColumns, int nRows);
        virtual void processKey(int key);
        virtual bool processButton(uint button, uint x, uint y);

        bool contains(int x, int y) const;
        int y() const;
        int x() const;
        int height() const;
        int width() const;

        bool hasFocus() const;
        void setHasFocus(bool active);

        bool isVisible() const;
        void setVisible(bool value);

        void setUserData(
                const S9sString  &key,
                const S9sVariant &value);

        S9sVariant userData(
                const S9sString  &key) const;

    protected:
        int            m_x;
        int            m_y;
        int            m_width;
        int            m_height;
        bool           m_isVisible;        
        bool           m_hasFocus;
        S9sVariantMap  m_userData;
};

