#include "xCommon.h"
#include "zPlayerInput.h"

#include "xMath.h"

xTWEAKABLE U32 BUTTON_JUMP = 0x10000;
xTWEAKABLE U32 BUTTON_ACTION_MOVE = 0x80000;
xTWEAKABLE U32 BUTTON_CONTEXT_MOVE = 0x20000;
xTWEAKABLE U32 BUTTON_OTHER_MOVE = 0x40000;

zPlayerInput::zPlayerInput()
{
    mPad = NULL;
}

zPlayerInput::~zPlayerInput()
{
}

void zPlayerInput::Clear()
{
    memset(&on, 0, sizeof(on));
    memset(&pressed, 0, sizeof(pressed));
    memset(&released, 0, sizeof(released));
    memset(&stick1, 0, sizeof(stick1));
    memset(&stick2, 0, sizeof(stick2));
    mActionTimer = 0.0f;
}

void zPlayerInput::Release()
{
    mPad = NULL;
}

void zPlayerInput::SetPad(zPad* pad)
{
    xASSERT_L(39, pad);

    xASSERT_L(42, !mPad || mPad->GetPadPort() == pad->GetPadPort());
    mPad = pad;
    mPad->Clear();
}

void zPlayerInput::Update(F32 dt)
{
    HumanPlayerUpdate(dt);
}

void zPlayerInput::HumanPlayerUpdate(F32 dt)
{
    if (!mPad)
        return;

#if DEBUG || RELEASE
    if (mPad->GetDebugPad()->Enabled() && mPad->GetDebugPad()->GetPadPort() == mPad->GetPadPort())
        return;
#endif

    mPad->GetAnalog1()->Get(stick1.x, stick1.y);
    mPad->GetAnalog2()->Get(stick2.x, stick2.y);

    on.jump = mPad->On(BUTTON_JUMP);
    on.actionMove = mPad->On(BUTTON_ACTION_MOVE);
    on.contextMove = mPad->On(BUTTON_CONTEXT_MOVE);
    on.otherMove = mPad->On(BUTTON_OTHER_MOVE);

    pressed.jump = mPad->Pressed(BUTTON_JUMP);
    pressed.actionMove = mPad->Pressed(BUTTON_ACTION_MOVE);
    pressed.contextMove = mPad->Pressed(BUTTON_CONTEXT_MOVE);
    pressed.otherMove = mPad->Pressed(BUTTON_OTHER_MOVE);

    released.jump = mPad->Released(BUTTON_JUMP);
    released.actionMove = mPad->Released(BUTTON_ACTION_MOVE);
    released.contextMove = mPad->Released(BUTTON_CONTEXT_MOVE);
    released.otherMove = mPad->Released(BUTTON_OTHER_MOVE);

    if (mPad->Pressed(BUTTON_JUMP | BUTTON_ACTION_MOVE | BUTTON_CONTEXT_MOVE | BUTTON_OTHER_MOVE))
        mActionTimer = 0.0f;
    else
        mActionTimer += dt;
    F32 padMovement = stick1.x * stick1.x + stick1.y * stick1.y;
    if (padMovement > 0.2f)
        mActionTimer = 0.0f;
}

void zPlayerInput::GetAnalog1(F32& x, F32& y, F32& angle, F32& mag) const
{
    x = stick1.x;
    y = stick1.y;
    angle = xatan2(x, y);
    mag = MAX(xabs(x), xabs(y));
}

zPad* zPlayerInput::GetPad() const
{
    return mPad;
}
