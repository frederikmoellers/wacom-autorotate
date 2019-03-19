# wacom-autorotate
## Automatically rotate your touchscreen devices together with your screen

This program is intended for use on convertibles that feature automatic screen rotation. It reacts on rotation events from the X server and rotates all xf86-input-wacom devices accordingly.

## COMPILING
`gcc -o wacom-autorotate -lX11 -lXi -lXrandr wacom-autorotate.c`

## INSTALLING
1. Place the resulting binary `wacom-autorotate` somewhere and make it executable.
2. Then configure your graphical desktop to automatically launch it at login.
3. Profit!
