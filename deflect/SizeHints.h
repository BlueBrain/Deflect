/*********************************************************************/
/* Copyright (c) 2015, EPFL/Blue Brain Project                       */
/*                     Daniel.Nachbaur@epfl.ch                       */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#ifndef DEFLECT_SIZEHINTS_H
#define DEFLECT_SIZEHINTS_H

#include <deflect/config.h>

namespace deflect
{
/**
 * A struct that contains hints about minimum, maximum and preferred sizes of a
 * streamer which can be interpreted by the stream server accordingly.
 *
 * @version 1.2
 */
struct SizeHints
{
    /** value for an unspecified size value; streamer did not report any hint */
    static const unsigned int UNSPECIFIED_SIZE = 0;

    /** @name Minimum size */
    //@{
    unsigned int minWidth = UNSPECIFIED_SIZE;
    unsigned int minHeight = UNSPECIFIED_SIZE;
    //@}

    /** @name Maximum size */
    //@{
    unsigned int maxWidth = UNSPECIFIED_SIZE;
    unsigned int maxHeight = UNSPECIFIED_SIZE;
    //@}

    /** @name Preferred size */
    //@{
    unsigned int preferredWidth = UNSPECIFIED_SIZE;
    unsigned int preferredHeight = UNSPECIFIED_SIZE;
    //@}
};

/** @return true if rhs and this are equal for all sizes. */
inline bool operator==(const SizeHints& lhs, const SizeHints& rhs) NOEXCEPT
{
    return lhs.minWidth == rhs.minWidth && lhs.minHeight == rhs.minHeight &&
           lhs.maxWidth == rhs.maxWidth && lhs.maxHeight == rhs.maxHeight &&
           lhs.preferredWidth == rhs.preferredWidth &&
           lhs.preferredHeight == rhs.preferredHeight;
}

/** @return true if rhs and this not equal for any size. */
inline bool operator!=(const SizeHints& lhs, const SizeHints& rhs) NOEXCEPT
{
    return !(lhs == rhs);
}
}

#endif
