/*
    wacom-autorotate: Automatically rotate touchscreen devices
    Copyright (C) 2019  Frederik Möllers

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/extensions/randr.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xlib.h>
#include <xorg/wacom-properties.h>

/**
 * Warning: This program doesn't work under Wayland because it uses xf86-input-wacom functionality
 */

// the current rotation of the screen
Rotation current_rotation = RR_Rotate_0;
// the X display
Display *dpy;
// continue running the main loop?
int running = 1;
// the screen the application is watching
int screen;
// the root window of the "dpy" display
Window root;

/**
 * Signal handler for SIGINT and SIGTERM to quit the program
 */

void signal_handler(int signum)
{
    // TODO: Gracefully shut down without using exit(0)
    exit(0);
}
const struct sigaction signal_action_struct = {
    .sa_handler = &signal_handler
};

/**
 * Rotates all Wacom devices by the specified amount.
 */
void rotate_wacom_devices(Rotation rotation)
{
    // the list of devices
	XDeviceInfo	*info;
    // the number of devices
	int ndevices;
    // an identifier for wacom devices
	Atom wacom_prop;
    // an opened Xinput device
    XDevice *dev;
    // a device's atoms
    Atom *atoms;
    // the number of atoms of a device
    int natoms;
    // the prop for the rotation of a specific wacom device
    Atom prop = 0;
    // the type of the rotation property
    Atom type;
    // more attributes of the rotation property
    int format;
    unsigned char* data = NULL;
    unsigned long nitems, bytes_after;
    // the rotation as a wacom property
    int rotation_target = 0;
    switch(rotation)
    {
        case RR_Rotate_90:
            rotation_target = 2;
            break;
        case RR_Rotate_180:
            rotation_target = 3;
            break;
        case RR_Rotate_270:
            rotation_target = 1;
            break;
    }

	wacom_prop = XInternAtom(dpy, "Wacom Tool Type", True);
	if (wacom_prop == None)
		return;

	info = XListInputDevices(dpy, &ndevices);

	for (size_t i = 0; i < ndevices; i++)
	{
		if (info[i].use == IsXPointer || info[i].use == IsXKeyboard)
			continue;
        dev = XOpenDevice(dpy, info[i].id);
        atoms = XListDeviceProperties(dpy, dev, &natoms);
        for(size_t j = 0; j < natoms; j++)
        {
            if(atoms[j] != wacom_prop)
                continue;
            prop = XInternAtom(dpy, WACOM_PROP_ROTATION, True);
            if(!prop)
                continue;
            XGetDeviceProperty(dpy, dev, prop, 0, 1000, False, AnyPropertyType, &type, &format, &nitems, &bytes_after, &data);
            // from https://github.com/linuxwacom/xf86-input-wacom/blob/2062126997bfe2c014533873d73a95198b335305/tools/xsetwacom.c#L1684
            if(nitems == 0 || format != 8)
            {
                fprintf(stderr, "Property '%s' of device '%s' has no or wrong value!", WACOM_PROP_ROTATION, info[i].name);
                continue;
            }
            *data = rotation_target;
            XChangeDeviceProperty(dpy, dev, prop, type, format, PropModeReplace, data, nitems);
        }
        XCloseDevice(dpy, dev);
        XFree(atoms);
	}

    XFreeDeviceList(info);
    return;
}

int main(int argc, char* argv[])
{
    // the last captured event
    XEvent event;
    // whether we have the XRandR extension
    int have_rr;
    // RandR extension variables
    int rr_event_base, rr_error_base;
    // open the X display
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf (stderr, "Unable to open display!\n");
        exit (1);
    }
    // get the root window
    screen = DefaultScreen (dpy);
	root = RootWindow(dpy, screen);

    // check for the RandR extension
    have_rr = XRRQueryExtension (dpy, &rr_event_base, &rr_error_base);
    if (!have_rr) {
        fprintf(stderr, "Cannot find XRandR extension!\n");
        exit(1);
    }

    // I *think* this selects the types of events to listen for
    // first, it checks the version of the RandR extension and then subscribes to all events supported by that version
    int rr_major, rr_minor;
    if (XRRQueryVersion (dpy, &rr_major, &rr_minor)) {
        long rr_mask = RRScreenChangeNotifyMask | RRCrtcChangeNotifyMask | RROutputChangeNotifyMask | RROutputPropertyNotifyMask;
        if (rr_major == 1 && rr_minor <= 1) {
            rr_mask &= ~(RRCrtcChangeNotifyMask |
                            RROutputChangeNotifyMask |
                            RROutputPropertyNotifyMask);
        }
        XRRSelectInput (dpy, root, rr_mask);
    }

    if(sigaction(SIGINT, &signal_action_struct, NULL) != 0 ||
       sigaction(SIGTERM, &signal_action_struct, NULL) != 0)
    {
        fprintf(stderr, "Could not set up signal handler!\n");
        exit(1);
    }

    // main loop, wait for events and handle them
    while(running)
    {
        // get the next event
	    XNextEvent (dpy, &event);
        // if it's not a RandR screen change, ignore it
        if (event.type != rr_event_base + RRScreenChangeNotify)
            continue;
        // get the rotation of the event
        XRRScreenChangeNotifyEvent *scn_event = (XRRScreenChangeNotifyEvent *) &event;
        // if the rotation did not change, ignore it
        if(scn_event->rotation == current_rotation)
            continue;
        // save new rotation
        current_rotation = scn_event->rotation;
        // handle rotation
        rotate_wacom_devices(current_rotation);
    }
}
