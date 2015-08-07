/*********************************************************************/
/* Copyright (c) 2014-2015, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
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

#ifndef DEFLECT_COMMAND_H
#define DEFLECT_COMMAND_H

#include <deflect/api.h>
#include <deflect/types.h>
#include <deflect/CommandType.h>

#include <boost/noncopyable.hpp>

namespace deflect
{

/**
 * String format, prefix-base network command.
 */
class Command : public boost::noncopyable
{
public:
    /**
     * Constructor.
     * @param type The type of the command.
     * @param args The command arguments.
     */
    DEFLECT_API Command( CommandType type, const QString& args );

    /**
     * Constructor.
     * @param command A string-formatted command, as obtained by getCommand().
     */
    // cppcheck-suppress noExplicitConstructor
    DEFLECT_API Command( const QString& command );

    /** Destructor. */
    DEFLECT_API ~Command();

    /** Get the command type. */
    DEFLECT_API CommandType getType() const;

    /** Get the command arguments. */
    DEFLECT_API const QString& getArguments() const;

    /** Get the command in string format, typically for sending over network. */
    DEFLECT_API const QString& getCommand() const;

    /** Check if the Command is valid (i.e. it has a known type). */
    DEFLECT_API bool isValid() const;

private:
    class Impl;
    Impl* _impl;
};

}

#endif
