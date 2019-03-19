# wacom-autorotate
Automatically rotate your touchscreen devices together with your screen

## About
This program is intended for use on convertibles that feature automatic screen rotation. It reacts on rotation events from the X server and rotates all xf86-input-wacom devices accordingly.

## Compiling
You need the following packages (names are taken from Debian, YMMV): `libx11-dev libxi-dev libxrandr-dev`

Then, to compile

`gcc -o wacom-autorotate -lX11 -lXi -lXrandr wacom-autorotate.c`

## Installing
1. Place the resulting binary `wacom-autorotate` somewhere and make it executable.
2. Then configure your graphical desktop to automatically launch it at login.
3. Profit!
