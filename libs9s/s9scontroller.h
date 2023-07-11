/*
 * Severalnines Tools
 * Copyright (C) 2016  Severalnines AB
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

#include "s9sserver.h"

/**
 * A class that represents a Cmon Controller, a class that is used since the
 * Cmon HA is introduced.
 */
class S9sController : public S9sServer
{
    public:
        S9sController();
        S9sController(const S9sController &orig);
        S9sController(const S9sVariantMap &properties);

        virtual ~S9sController();

        S9sController &operator=(const S9sVariantMap &rhs);
       
        bool connectFailed() const;
        void setConnectFailed(bool value = true);
};


