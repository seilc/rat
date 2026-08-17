#include "xEntDrive.h"

void xEntDriveInit(xEntDrive* drv, xEnt* driven)
{
    xASSERT(30, drv);

    drv->flags = 0;
    drv->driven = driven;
    drv->driver = NULL;
    drv->influenceOfDriver = 0.0f;
    drv->introTime = 0.0f;
    drv->introTimer = 0.0f;
    drv->oldDriver = NULL;
    drv->influenceOfOldDriver = 0.0f;
    drv->outroTime = 0.0f;
    drv->outroTimer = 0.0f;
}

void xEntDriveMount(xEntDrive* drv, xEnt* driver, F32 mt, const xCollis* coll)
{
    xASSERT(46, drv);
    xASSERT(47, driver);
    xASSERT(48, drv->driven);
    xASSERT(49, drv->driven->frame);

    // Hack to force xVec3::operator= to link after xCollis::tri_data::operator=
    if (0) {
        xVec3 a, b;
        a = b;
    }

    drv->dLoc = 0.0f;

    if (driver == drv->oldDriver && drv->influenceOfOldDriver) {
        drv->driver = driver;
        driver->driving_count++;
        if (mt < 0.0f) {
            drv->influenceOfDriver = 1.0f;
            drv->introTimer = 0.0f;
        } else {
            drv->influenceOfDriver = drv->influenceOfOldDriver;
            drv->introTimer = mt * (1.0f - drv->influenceOfDriver);
        }
        drv->introTime = mt;
        drv->oldDriver = NULL;
        drv->influenceOfOldDriver = 0.0f;
        drv->outroTime = 0.0f;
        drv->outroTimer = 0.0f;
    } else {
        drv->driver = driver;
        driver->driving_count++;
        if (mt < 0.0f) {
            drv->influenceOfDriver = 1.0f;
            drv->introTimer = 0.0f;
        } else {
            drv->influenceOfDriver = 0.0f;
            drv->introTimer = mt;
        }
        drv->introTime = mt;
    }

    if (drv->flags & 0x1) {
        xVec3 euler;
        xMat3x3 a_descaled;
        F32 dummy;
        xVec3NormalizeMacro(&a_descaled.right, &drv->driver->model->Mat->right, &dummy);
        xVec3NormalizeMacro(&a_descaled.up, &drv->driver->model->Mat->up, &dummy);
        xVec3NormalizeMacro(&a_descaled.at, &drv->driver->model->Mat->at, &dummy);
        xMat3x3GetEuler(&a_descaled, &euler);
        drv->yawInDriver = euler.x;
    }

    if (coll && (coll->flags & k_HIT_CALC_TRI)) {
        drv->flags |= 0x2;
        (xCollis::tri_data&)drv->tri = coll->tri;
        drv->tri.loc = xCollisTriHit(drv->tri, *driver->model);
        xMat4x3ToLocal(&drv->tri.loc, (xMat4x3*)drv->driver->model->Mat, &drv->tri.loc);
        drv->tri.coll = coll;
        drv->tri.trioldmat = *(xMat4x3*)drv->driver->model->Mat;
    }

    xVec3Copy(&drv->lastDrivenPos, &drv->driven->frame->mat.pos);
    xMat4x3ToLocal(&drv->drivenPosInDriver, (xMat4x3*)drv->driver->model->Mat, &drv->lastDrivenPos);
}

void xEntDriveDismount(xEntDrive* drv, F32 dmt)
{
    xASSERT(118, drv);

    xEnt* pDriver = drv->driver;
    if (!pDriver) {
        return;
    }

    pDriver->driving_count--;

    drv->oldDriver = pDriver;
    drv->influenceOfOldDriver = drv->influenceOfDriver;
    drv->outroTime = dmt;
    drv->outroTimer = dmt * drv->influenceOfOldDriver;
    drv->driver = NULL;
    drv->influenceOfDriver = 0.0f;
    drv->introTime = 0.0f;
    drv->introTimer = 0.0f;
    drv->flags &= ~0x2;
    
    xVec3Copy(&drv->lastDrivenPos, &drv->driven->frame->mat.pos);
    xMat4x3ToLocal(&drv->drivenPosInOldDriver, (xMat4x3*)drv->oldDriver->model->Mat, &drv->lastDrivenPos);
}

void xEntDriveUpdate(xEntDrive* drv, xScene*, F32 dt, const xCollis*)
{
    xASSERT(150, drv);

    if (!drv->oldDriver && !drv->driver) {
        return;
    }

    xASSERT(156, drv->driven);
    xASSERT(157, drv->driven->frame);

    if (drv->outroTimer > 0.0f) {
        drv->outroTimer -= dt;
        if (drv->outroTimer <= 0.0f) {
            drv->influenceOfOldDriver = 0.0f;
            drv->outroTimer = 0.0f;
        } else if (xfeq0(drv->outroTime)) {
            drv->influenceOfOldDriver = 0.0f;
        } else {
            drv->influenceOfOldDriver = drv->outroTimer / drv->outroTime;
        }
    } else {
        drv->influenceOfOldDriver = 0.0f;
    }

    if (drv->introTimer > 0.0f) {
        drv->introTimer -= dt;
        if (drv->introTimer <= 0.0f) {
            drv->influenceOfDriver = 1.0f;
            drv->introTimer = 0.0f;
        } else {
            drv->influenceOfDriver = 1.0f - drv->introTimer / drv->introTime;
        }
    }

    if (!drv->influenceOfOldDriver && !drv->influenceOfDriver) {
        drv->driver = NULL;
        drv->oldDriver = NULL;
        return;
    }

    if (drv->influenceOfDriver && (drv->flags & 0x1)) {
        xVec3 euler;
        xMat3x3 rot, a_descaled;
        F32 dummy;
        xVec3NormalizeMacro(&a_descaled.right, &drv->driver->model->Mat->right, &dummy);
        xVec3NormalizeMacro(&a_descaled.up, &drv->driver->model->Mat->up, &dummy);
        xVec3NormalizeMacro(&a_descaled.at, &drv->driver->model->Mat->at, &dummy);
        xMat3x3GetEuler(&a_descaled, &euler);
        xMat3x3RotY(&rot, drv->influenceOfDriver * (euler.x - drv->yawInDriver));
        xMat3x3Mul(&drv->driven->frame->mat, &drv->driven->frame->mat, &rot);
        drv->yawInDriver = euler.x;
    }

    drv->dLoc = 0.0f;

    xVec3 newq;

    if (drv->influenceOfOldDriver && drv->oldDriver) {
        xMat4x3ToWorld(&newq, (xMat4x3*)drv->oldDriver->model->Mat, &drv->drivenPosInOldDriver);
        xVec3Sub(drv->driven->frame->setDpos(), newq, drv->lastDrivenPos);
        xVec3SMulBy(drv->driven->frame->setDpos(), drv->influenceOfOldDriver);
        xVec3AddTo(drv->driven->frame->mat.pos, drv->driven->frame->getDpos());
        drv->dLoc += drv->driven->frame->getDpos();
    }

    if (drv->influenceOfDriver) {
        if (drv->flags & 0x2) {
            xModelInstance& m = *drv->driver->model;
            if (xModelAnimCollDirty(m)) {
                xModelAnimCollRefresh(m);
            }

            xVec3 world_loc, new_loc;
            xMat4x3ToWorld(&world_loc, &drv->tri.trioldmat, &drv->tri.loc);
            new_loc = xCollisTriHit(drv->tri, m);
            drv->driven->frame->setDpos().Sub(new_loc, world_loc);

            if (drv->tri.index != drv->tri.coll->tri.index
                || !xeq(drv->tri.r, drv->tri.coll->tri.r, 0.1f)
                || !xeq(drv->tri.d, drv->tri.coll->tri.d, 0.1f)) {
                (xCollis::tri_data&)drv->tri = drv->tri.coll->tri;
            }

            xMat4x3 oldmat = *(xMat4x3*)m.Mat;
            *(xMat4x3*)m.Mat = *(xMat4x3*)drv->driver->model->Mat;
            drv->tri.loc = xCollisTriHit(drv->tri, m);
            xMat4x3ToLocal(&drv->tri.loc, (xMat4x3*)drv->driver->model->Mat, &drv->tri.loc);
            *(xMat4x3*)m.Mat = oldmat;
        } else {
            xMat4x3ToWorld(&newq, (xMat4x3*)drv->driver->model->Mat, &drv->drivenPosInDriver);
            xVec3Sub(drv->driven->frame->setDpos(), newq, drv->lastDrivenPos);
        }

        drv->driven->frame->setDpos() *= drv->influenceOfDriver;
        drv->dLoc += drv->driven->frame->getDpos();
        drv->driven->frame->mat.pos += drv->driven->frame->getDpos();
        drv->driven->frame->setDpos().assign(0.0f);

        if (drv->driven->model) {
            (xVec3&)drv->driven->model->Mat->pos = drv->driven->frame->mat.pos;
        }
    }

    xVec3Copy(&drv->lastDrivenPos, &drv->driven->frame->mat.pos);

    if (drv->influenceOfOldDriver) {
        xMat4x3ToLocal(&drv->drivenPosInOldDriver, (xMat4x3*)drv->oldDriver->model->Mat, &drv->lastDrivenPos);
    }

    if (drv->influenceOfDriver) {
        xMat4x3ToLocal(&drv->drivenPosInDriver, (xMat4x3*)drv->driver->model->Mat, &drv->lastDrivenPos);
        drv->tri.trioldmat = *(xMat4x3*)drv->driver->model->Mat;
    }
}

void xEntDriveSimpleInit(xEntDriveSimple* drv, xMat4x3* mat)
{
    drv->drivenMat = mat;
    drv->driver = NULL;
    drv->drivenPosInDriver.assign(0.0f);
    drv->lastDrivenPos.assign(0.0f);
    drv->dpos.assign(0.0f);
}

void xEntDriveSimpleMount(xEntDriveSimple* drv, xEnt* driver)
{
    xASSERT(318, driver);

    drv->driver = driver;
    xVec3Copy(&drv->lastDrivenPos, &drv->drivenMat->pos);
    xMat4x3ToLocal(&drv->drivenPosInDriver, (xMat4x3*)drv->driver->model->Mat, &drv->lastDrivenPos);
}

void xEntDriveSimpleUpdate(xEntDriveSimple* drv, F32)
{
    xVec3 newq;
    xMat4x3ToWorld(&newq, (xMat4x3*)drv->driver->model->Mat, &drv->drivenPosInDriver);
    xVec3Sub(&drv->dpos, &newq, &drv->lastDrivenPos);
}

void xEntDriveSimplePostUpdate(xEntDriveSimple* drv)
{
    xVec3Copy(&drv->lastDrivenPos, &drv->drivenMat->pos);
    xMat4x3ToLocal(&drv->drivenPosInDriver, (xMat4x3*)drv->driver->model->Mat, &drv->lastDrivenPos);
}
