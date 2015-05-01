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

class AppNapSuspenderPrivate
{
public:
    AppNapSuspenderPrivate()
        : activityId(nil)
    {}

    id<NSObject> activityId;
};

AppNapSuspender::AppNapSuspender() :
    impl_(new AppNapSuspenderPrivate)
{
}

AppNapSuspender::~AppNapSuspender()
{
    resume();
    delete impl_;
}

void AppNapSuspender::suspend()
{
    if (impl_->activityId)
        return;

    if ([[NSProcessInfo processInfo] respondsToSelector:@selector(beginActivityWithOptions:reason:)])
    {
        impl_->activityId = [[NSProcessInfo processInfo] beginActivityWithOptions: NSActivityUserInitiated reason:@"Good reason"];
        [impl_->activityId retain];
    }
}

void AppNapSuspender::resume()
{
    if(!impl_->activityId)
        return;

    if([[NSProcessInfo processInfo] respondsToSelector:@selector(endActivity:)])
    {
        [[NSProcessInfo processInfo] endActivity:impl_->activityId];
        [impl_->activityId release];
        impl_->activityId = nil;
    }
}
