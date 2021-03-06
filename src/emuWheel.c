/*
* xserver-xorg-input-evdev-multitouch
*
* Contact: Sung-Jin Park <sj76.park@samsung.com>
*          Sangjin LEE <lsj119@samsung.com>
*
* Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
*
* Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
* Copyright 1993 by David Dawes <dawes@xfree86.org>
* Copyright 2002 by SuSE Linux AG, Author: Egbert Eich
* Copyright 1994-2002 by The XFree86 Project, Inc.
* Copyright 2002 by Paul Elliott
* (Ported from xf86-input-mouse, above copyrights taken from there)
* Copyright 2008 by Chris Salch
* Copyright © 2008 Red Hat, Inc.
*
* Permission to use, copy, modify, distribute, and sell this software
* and its documentation for any purpose is hereby granted without
* fee, provided that the above copyright notice appear in all copies
* and that both that copyright notice and this permission notice
* appear in supporting documentation, and that the name of the authors
* not be used in advertising or publicity pertaining to distribution of the
* software without specific, written prior permission.  The authors make no
* representations about the suitability of this software for any
* purpose.  It is provided "as is" without express or implied
* warranty.
*
* THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
* INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
* NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
* CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
* OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
* NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
* CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*
*/

/* Mouse wheel emulation code. */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xatom.h>
#include <xf86.h>
#include <xf86Xinput.h>
#include <exevents.h>

#include <evdevmultitouch-properties.h>
#include "evdevmultitouch.h"

#define WHEEL_NOT_CONFIGURED 0

#ifdef HAVE_PROPERTIES
static Atom prop_wheel_emu      = 0;
static Atom prop_wheel_axismap  = 0;
static Atom prop_wheel_inertia  = 0;
static Atom prop_wheel_timeout  = 0;
static Atom prop_wheel_button   = 0;
#endif

/* Local Funciton Prototypes */
static BOOL EvdevMultitouchWheelEmuHandleButtonMap(InputInfoPtr pInfo, WheelAxisPtr pAxis, char *axis_name);
static int EvdevMultitouchWheelEmuInertia(InputInfoPtr pInfo, WheelAxisPtr axis, int value);

/* Filter mouse button events */
BOOL
EvdevMultitouchWheelEmuFilterButton(InputInfoPtr pInfo, unsigned int button, int value)
{
    EvdevMultitouchPtr pEvdevMultitouch = (EvdevMultitouchPtr)pInfo->private;
    int ms;

    /* Has wheel emulation been configured to be enabled? */
    if (!pEvdevMultitouch->emulateWheel.enabled)
	return FALSE;

    /* Check for EmulateWheelButton */
    if (pEvdevMultitouch->emulateWheel.button == button) {
	pEvdevMultitouch->emulateWheel.button_state = value;

        if (value)
            /* Start the timer when the button is pressed */
            pEvdevMultitouch->emulateWheel.expires = pEvdevMultitouch->emulateWheel.timeout +
                                           GetTimeInMillis();
        else {
            ms = pEvdevMultitouch->emulateWheel.expires - GetTimeInMillis();
            if (ms > 0) {
                /*
                 * If the button is released early enough emit the button
                 * press/release events
                 */
                EvdevMultitouchQueueButtonClicks(pInfo, button, 1);
            }
        }

	return TRUE;
    }

    /* Don't care about this event */
    return FALSE;
}

/* Filter mouse wheel events */
BOOL
EvdevMultitouchWheelEmuFilterMotion(InputInfoPtr pInfo, struct input_event *pEv)
{
    EvdevMultitouchPtr pEvdevMultitouch = (EvdevMultitouchPtr)pInfo->private;
    WheelAxisPtr pAxis = NULL, pOtherAxis = NULL;
    int value = pEv->value;

    /* Has wheel emulation been configured to be enabled? */
    if (!pEvdevMultitouch->emulateWheel.enabled)
	return FALSE;

    /* Handle our motion events if the emuWheel button is pressed
     * wheel button of 0 means always emulate wheel.
     */
    if (pEvdevMultitouch->emulateWheel.button_state || !pEvdevMultitouch->emulateWheel.button) {
        /* Just return if the timeout hasn't expired yet */
        if (pEvdevMultitouch->emulateWheel.button)
        {
            int ms = pEvdevMultitouch->emulateWheel.expires - GetTimeInMillis();
            if (ms > 0)
                return TRUE;
        }

	/* We don't want to intercept real mouse wheel events */
	switch(pEv->code) {
	case REL_X:
	    pAxis = &(pEvdevMultitouch->emulateWheel.X);
	    pOtherAxis = &(pEvdevMultitouch->emulateWheel.Y);
	    break;

	case REL_Y:
	    pAxis = &(pEvdevMultitouch->emulateWheel.Y);
	    pOtherAxis = &(pEvdevMultitouch->emulateWheel.X);
	    break;

	default:
	    break;
	}

	/* If we found REL_X or REL_Y, emulate a mouse wheel.
           Reset the inertia of the other axis when a scroll event was sent
           to avoid the buildup of erroneous scroll events if the user
           doesn't move in a perfectly straight line.
         */
	if (pAxis)
	{
	    if (EvdevMultitouchWheelEmuInertia(pInfo, pAxis, value))
		pOtherAxis->traveled_distance = 0;
	}

	/* Eat motion events while emulateWheel button pressed. */
	return TRUE;
    }

    return FALSE;
}

/* Simulate inertia for our emulated mouse wheel.
   Returns the number of wheel events generated.
 */
static int
EvdevMultitouchWheelEmuInertia(InputInfoPtr pInfo, WheelAxisPtr axis, int value)
{
    EvdevMultitouchPtr pEvdevMultitouch = (EvdevMultitouchPtr)pInfo->private;
    int button;
    int inertia;
    int rc = 0;

    /* if this axis has not been configured, just eat the motion */
    if (!axis->up_button)
	return rc;

    axis->traveled_distance += value;

    if (axis->traveled_distance < 0) {
	button = axis->up_button;
	inertia = -pEvdevMultitouch->emulateWheel.inertia;
    } else {
	button = axis->down_button;
	inertia = pEvdevMultitouch->emulateWheel.inertia;
    }

    /* Produce button press events for wheel motion */
    while(abs(axis->traveled_distance) > pEvdevMultitouch->emulateWheel.inertia) {
	axis->traveled_distance -= inertia;
	EvdevMultitouchQueueButtonClicks(pInfo, button, 1);
	rc++;
    }
    return rc;
}

/* Handle button mapping here to avoid code duplication,
returns true if a button mapping was found. */
static BOOL
EvdevMultitouchWheelEmuHandleButtonMap(InputInfoPtr pInfo, WheelAxisPtr pAxis, char* axis_name)
{
    EvdevMultitouchPtr pEvdevMultitouch = (EvdevMultitouchPtr)pInfo->private;
    char *option_string;

    pAxis->up_button = WHEEL_NOT_CONFIGURED;

    /* Check to see if there is configuration for this axis */
    option_string = xf86SetStrOption(pInfo->options, axis_name, NULL);
    if (option_string) {
	int up_button = 0;
	int down_button = 0;
	char *msg = NULL;

	if ((sscanf(option_string, "%d %d", &up_button, &down_button) == 2) &&
	    ((up_button > 0) && (up_button <= EVDEVMULTITOUCH_MAXBUTTONS)) &&
	    ((down_button > 0) && (down_button <= EVDEVMULTITOUCH_MAXBUTTONS))) {

	    /* Use xstrdup to allocate a string for us */
	    msg = xstrdup("buttons XX and YY");

	    if (msg)
		sprintf(msg, "buttons %d and %d", up_button, down_button);

	    pAxis->up_button = up_button;
	    pAxis->down_button = down_button;

	    /* Update the number of buttons if needed */
	    if (up_button > pEvdevMultitouch->num_buttons) pEvdevMultitouch->num_buttons = up_button;
	    if (down_button > pEvdevMultitouch->num_buttons) pEvdevMultitouch->num_buttons = down_button;

	} else {
	    xf86Msg(X_WARNING, "%s: Invalid %s value:\"%s\"\n",
		    pInfo->name, axis_name, option_string);

	}

	/* Clean up and log what happened */
	if (msg) {
	    xf86Msg(X_CONFIG, "%s: %s: %s\n",pInfo->name, axis_name, msg);
	    free(msg);
	    return TRUE;
	}
    }
    return FALSE;
}

/* Setup the basic configuration options used by mouse wheel emulation */
void
EvdevMultitouchWheelEmuPreInit(InputInfoPtr pInfo)
{
    EvdevMultitouchPtr pEvdevMultitouch = (EvdevMultitouchPtr)pInfo->private;
    char val[4];
    int wheelButton;
    int inertia;
    int timeout;

    val[0] = 0;
    val[1] = 0;

    if (xf86SetBoolOption(pInfo->options, "EmulateWheel", FALSE)) {
	pEvdevMultitouch->emulateWheel.enabled = TRUE;
    } else
        pEvdevMultitouch->emulateWheel.enabled = FALSE;

    wheelButton = xf86SetIntOption(pInfo->options, "EmulateWheelButton", 4);

    if ((wheelButton < 0) || (wheelButton > EVDEVMULTITOUCH_MAXBUTTONS)) {
        xf86Msg(X_WARNING, "%s: Invalid EmulateWheelButton value: %d\n",
                pInfo->name, wheelButton);
        xf86Msg(X_WARNING, "%s: Wheel emulation disabled.\n", pInfo->name);

        pEvdevMultitouch->emulateWheel.enabled = FALSE;
    }

    pEvdevMultitouch->emulateWheel.button = wheelButton;

    inertia = xf86SetIntOption(pInfo->options, "EmulateWheelInertia", 10);

    if (inertia <= 0) {
        xf86Msg(X_WARNING, "%s: Invalid EmulateWheelInertia value: %d\n",
                pInfo->name, inertia);
        xf86Msg(X_WARNING, "%s: Using built-in inertia value.\n",
                pInfo->name);

        inertia = 10;
    }

    pEvdevMultitouch->emulateWheel.inertia = inertia;

    timeout = xf86SetIntOption(pInfo->options, "EmulateWheelTimeout", 200);

    if (timeout < 0) {
        xf86Msg(X_WARNING, "%s: Invalid EmulateWheelTimeout value: %d\n",
                pInfo->name, timeout);
        xf86Msg(X_WARNING, "%s: Using built-in timeout value.\n",
                pInfo->name);

        timeout = 200;
    }

    pEvdevMultitouch->emulateWheel.timeout = timeout;

    /* Configure the Y axis or default it */
    if (!EvdevMultitouchWheelEmuHandleButtonMap(pInfo, &(pEvdevMultitouch->emulateWheel.Y),
                "YAxisMapping")) {
        /* Default the Y axis to sane values */
        pEvdevMultitouch->emulateWheel.Y.up_button = 4;
        pEvdevMultitouch->emulateWheel.Y.down_button = 5;

        /* Simpler to check just the largest value in this case */
        /* XXX: we should post this to the server */
        if (5 > pEvdevMultitouch->num_buttons)
            pEvdevMultitouch->num_buttons = 5;

        /* Display default Configuration */
        xf86Msg(X_CONFIG, "%s: YAxisMapping: buttons %d and %d\n",
                pInfo->name, pEvdevMultitouch->emulateWheel.Y.up_button,
                pEvdevMultitouch->emulateWheel.Y.down_button);
    }


    /* This axis should default to an unconfigured state as most people
       are not going to expect a Horizontal wheel. */
    EvdevMultitouchWheelEmuHandleButtonMap(pInfo, &(pEvdevMultitouch->emulateWheel.X),
            "XAxisMapping");

    /* Used by the inertia code */
    pEvdevMultitouch->emulateWheel.X.traveled_distance = 0;
    pEvdevMultitouch->emulateWheel.Y.traveled_distance = 0;

    xf86Msg(X_CONFIG, "%s: EmulateWheelButton: %d, "
            "EmulateWheelInertia: %d, "
            "EmulateWheelTimeout: %d\n",
            pInfo->name, pEvdevMultitouch->emulateWheel.button, inertia, timeout);
}

#ifdef HAVE_PROPERTIES
static int
EvdevMultitouchWheelEmuSetProperty(DeviceIntPtr dev, Atom atom, XIPropertyValuePtr val,
                         BOOL checkonly)
{
    InputInfoPtr pInfo  = dev->public.devicePrivate;
    EvdevMultitouchPtr     pEvdevMultitouch = pInfo->private;

    if (atom == prop_wheel_emu)
    {
        if (val->format != 8 || val->size != 1 || val->type != XA_INTEGER)
            return BadMatch;

        if (!checkonly)
        {
            pEvdevMultitouch->emulateWheel.enabled = *((BOOL*)val->data);
            /* Don't enable with zero inertia, otherwise we may get stuck in an
             * infinite loop */
            if (pEvdevMultitouch->emulateWheel.inertia <= 0)
            {
                pEvdevMultitouch->emulateWheel.inertia = 10;
                /* We may get here before the property is actually enabled */
                if (prop_wheel_inertia)
                    XIChangeDeviceProperty(dev, prop_wheel_inertia, XA_INTEGER,
                            16, PropModeReplace, 1,
                            &pEvdevMultitouch->emulateWheel.inertia, TRUE);
            }
        }
    }
    else if (atom == prop_wheel_button)
    {
        int bt;

        if (val->format != 8 || val->size != 1 || val->type != XA_INTEGER)
            return BadMatch;

        bt = *((CARD8*)val->data);

        if (bt < 0 || bt >= EVDEVMULTITOUCH_MAXBUTTONS)
            return BadValue;

        if (!checkonly)
            pEvdevMultitouch->emulateWheel.button = bt;
    } else if (atom == prop_wheel_axismap)
    {
        if (val->format != 8 || val->size != 4 || val->type != XA_INTEGER)
            return BadMatch;

        if (!checkonly)
        {
            pEvdevMultitouch->emulateWheel.X.up_button = *((CARD8*)val->data);
            pEvdevMultitouch->emulateWheel.X.down_button = *(((CARD8*)val->data) + 1);
            pEvdevMultitouch->emulateWheel.Y.up_button = *(((CARD8*)val->data) + 2);
            pEvdevMultitouch->emulateWheel.Y.down_button = *(((CARD8*)val->data) + 3);
        }
    } else if (atom == prop_wheel_inertia)
    {
        int inertia;

        if (val->format != 16 || val->size != 1 || val->type != XA_INTEGER)
            return BadMatch;

        inertia = *((CARD16*)val->data);

        if (inertia < 0)
            return BadValue;

        if (!checkonly)
            pEvdevMultitouch->emulateWheel.inertia = inertia;
    } else if (atom == prop_wheel_timeout)
    {
        int timeout;

        if (val->format != 16 || val->size != 1 || val->type != XA_INTEGER)
            return BadMatch;

        timeout = *((CARD16*)val->data);

        if (timeout < 0)
            return BadValue;

        if (!checkonly)
            pEvdevMultitouch->emulateWheel.timeout = timeout;
    }
    return Success;
}

void
EvdevMultitouchWheelEmuInitProperty(DeviceIntPtr dev)
{
    InputInfoPtr pInfo  = dev->public.devicePrivate;
    EvdevMultitouchPtr     pEvdevMultitouch = pInfo->private;
    int          rc     = TRUE;
    char         vals[4];

    if (!dev->button) /* don't init prop for keyboards */
        return;

    prop_wheel_emu = MakeAtom(EVDEVMULTITOUCH_PROP_WHEEL, strlen(EVDEVMULTITOUCH_PROP_WHEEL), TRUE);
    rc = XIChangeDeviceProperty(dev, prop_wheel_emu, XA_INTEGER, 8,
                                PropModeReplace, 1,
                                &pEvdevMultitouch->emulateWheel.enabled, FALSE);
    if (rc != Success)
        return;

    XISetDevicePropertyDeletable(dev, prop_wheel_emu, FALSE);

    vals[0] = pEvdevMultitouch->emulateWheel.X.up_button;
    vals[1] = pEvdevMultitouch->emulateWheel.X.down_button;
    vals[2] = pEvdevMultitouch->emulateWheel.Y.up_button;
    vals[3] = pEvdevMultitouch->emulateWheel.Y.down_button;

    prop_wheel_axismap = MakeAtom(EVDEVMULTITOUCH_PROP_WHEEL_AXES, strlen(EVDEVMULTITOUCH_PROP_WHEEL_AXES), TRUE);
    rc = XIChangeDeviceProperty(dev, prop_wheel_axismap, XA_INTEGER, 8,
                                PropModeReplace, 4, vals, FALSE);

    if (rc != Success)
        return;

    XISetDevicePropertyDeletable(dev, prop_wheel_axismap, FALSE);

    prop_wheel_inertia = MakeAtom(EVDEVMULTITOUCH_PROP_WHEEL_INERTIA, strlen(EVDEVMULTITOUCH_PROP_WHEEL_INERTIA), TRUE);
    rc = XIChangeDeviceProperty(dev, prop_wheel_inertia, XA_INTEGER, 16,
                                PropModeReplace, 1,
                                &pEvdevMultitouch->emulateWheel.inertia, FALSE);
    if (rc != Success)
        return;

    XISetDevicePropertyDeletable(dev, prop_wheel_inertia, FALSE);

    prop_wheel_timeout = MakeAtom(EVDEVMULTITOUCH_PROP_WHEEL_TIMEOUT, strlen(EVDEVMULTITOUCH_PROP_WHEEL_TIMEOUT), TRUE);
    rc = XIChangeDeviceProperty(dev, prop_wheel_timeout, XA_INTEGER, 16,
                                PropModeReplace, 1,
                                &pEvdevMultitouch->emulateWheel.timeout, FALSE);
    if (rc != Success)
        return;

    XISetDevicePropertyDeletable(dev, prop_wheel_timeout, FALSE);

    prop_wheel_button = MakeAtom(EVDEVMULTITOUCH_PROP_WHEEL_BUTTON, strlen(EVDEVMULTITOUCH_PROP_WHEEL_BUTTON), TRUE);
    rc = XIChangeDeviceProperty(dev, prop_wheel_button, XA_INTEGER, 8,
                                PropModeReplace, 1,
                                &pEvdevMultitouch->emulateWheel.button, FALSE);
    if (rc != Success)
        return;

    XISetDevicePropertyDeletable(dev, prop_wheel_button, FALSE);

    XIRegisterPropertyHandler(dev, EvdevMultitouchWheelEmuSetProperty, NULL, NULL);
}
#endif
