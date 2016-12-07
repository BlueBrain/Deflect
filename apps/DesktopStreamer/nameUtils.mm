/* Copyright (c) 2016, EPFL/Blue Brain Project
 *                     Raphael Dumusc <raphael.dumusc@epfl.ch>
 *
 * This file is part of Deflect <https://github.com/BlueBrain/Deflect>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "nameUtils.h"

#import <Foundation/NSPathUtilities.h>

namespace nameutils
{

QString getFullUsername()
{
    const auto fullname = QString{ [NSFullUserName() UTF8String] };
    if( !fullname.isEmpty( ))
        return fullname;

    return qgetenv( "USER" );
}

}
