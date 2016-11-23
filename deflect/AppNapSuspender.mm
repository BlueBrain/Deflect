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

#include "AppNapSuspender.h"

#include <Foundation/NSProcessInfo.h>

// We check at runtime if beginActivityWithOptions and endActivity
// are available so that we can also compile using OSX SDK < 10.9
#pragma clang diagnostic ignored "-Wobjc-method-access"

#ifdef __MAC_OS_X_VERSION_MAX_ALLOWED
#if __MAC_OS_X_VERSION_MAX_ALLOWED < 1090
// NSActivityUserInitiated is undefined when compiling with OSX SDK < 10.9
#define NSActivityUserInitiated (0x00FFFFFFULL | (1ULL << 20))
#endif
#endif

namespace deflect
{

class AppNapSuspender::Impl
{
public:
    Impl()
        : activityId( nil )
    {}

    id<NSObject> activityId;
};

AppNapSuspender::AppNapSuspender() :
    _impl( new Impl )
{
}

AppNapSuspender::~AppNapSuspender()
{
    resume();
    delete _impl;
}

void AppNapSuspender::suspend()
{
    if( _impl->activityId )
        return;

    if( [[NSProcessInfo processInfo] respondsToSelector:@selector(beginActivityWithOptions:reason:)] )
    {
        _impl->activityId = [[NSProcessInfo processInfo] beginActivityWithOptions: NSActivityUserInitiated reason:@"Good reason"];
        [_impl->activityId retain];
    }
}

void AppNapSuspender::resume()
{
    if( !_impl->activityId )
        return;

    if( [[NSProcessInfo processInfo] respondsToSelector:@selector(endActivity:)] )
    {
        [[NSProcessInfo processInfo] endActivity:_impl->activityId];
        [_impl->activityId release];
        _impl->activityId = nil;
    }
}

}
