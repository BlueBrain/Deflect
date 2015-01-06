#ifndef APPNAPSUSPENDER_H
#define APPNAPSUSPENDER_H

class AppNapSuspenderPrivate;

/**
 * Suspend AppNap on OSX >= 10.9.
 */
class AppNapSuspender
{
public:
    /** Constructor. */
    AppNapSuspender();

    /** Destruct the object, resuming AppNap if it was suspended. */
    ~AppNapSuspender();

    /** Suspend AppNap. */
    void suspend();

    /** Resume AppNap */
    void resume();

private:
    AppNapSuspenderPrivate* impl_;
};

#endif // APPNAPSUSPENDER_H
