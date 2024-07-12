#ifndef IASYNC_H
#define IASYNC_H

void iAsyncLockRW();
void iAsyncUnlockRW();

struct iAsyncLockRWSentry
{
    iAsyncLockRWSentry()
    {
        iAsyncLockRW();
    }

    ~iAsyncLockRWSentry()
    {
        iAsyncUnlockRW();
    }
};

#endif
