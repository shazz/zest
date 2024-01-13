/*
 * joystick.c - joystick thread (software part)
 *
 * Copyright (c) 2020-2024 Francois Galea <fgalea at free.fr>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#define JOY_LED_FILE "/sys/class/leds/led0/brightness"

extern volatile uint32_t *parmreg;
extern volatile int thr_end;
int joystick_nb_connected = 0;

struct axis_state
{
    short x, y;
};

int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    /* Error, could not read full event. */
    return -1;
}

/**
 * Returns the number of axes on the controller or 0 if an error occurs.
 */
size_t get_axis_count(int fd)
{
    __u8 axes;

    if (ioctl(fd, JSIOCGAXES, &axes) == -1)
        return 0;

    return axes;
}

/**
 * Returns the number of buttons on the controller or 0 if an error occurs.
 */
size_t get_button_count(int fd)
{
    __u8 buttons;
    if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
        return 0;

    return buttons;
}

void joy_led_status(int fd, int st)
{
    if (fd >= 0)
    {
        char val = st ? '1' : '0';
        write(fd, &val, 1);
    }
}

size_t get_axis_state(struct js_event *event, struct axis_state axes[3])
{
    size_t axis = event->number / 2;

    if (axis < 3)
    {
        if (event->number % 2 == 0)
            axes[axis].x = event->value;
        else
            axes[axis].y = event->value;
    }

    return axis;
}


void *thread_joystick(void *arg)
{
    int js;
    struct js_event event;
    size_t axis;

    struct axis_state axes[3] = {0};

    int joystick_plugged = -1;

    char device[15];

    int joystick_nb = joystick_nb_connected;
    joystick_nb_connected += 1;

    int joyfd = open(JOY_LED_FILE, O_WRONLY | O_SYNC);
    joy_led_status(joyfd, 1);

    sprintf(device, "/dev/input/js%d", joystick_nb);
    printf("Opening icode for joystick %d on %s\n", joystick_nb, device);

    js = open(device, O_RDONLY);
    if (js != -1)
    {
        joystick_plugged = 1;
        printf("Joystick %d detected\n", joystick_nb);
    } else {
        printf("Cannot open %s\n", device);
    }

    /* This loop will exit if the controller is unplugged. */
    while (thr_end == 0)
    {
        if(joystick_plugged == 1) {
            read_event(js, &event);

            switch (event.type)
            {
                case JS_EVENT_BUTTON:
                    printf("Button of joystick %d: %u %s\n", joystick_nb, event.number, event.value ? "pressed" : "released");
                    parmreg[4 + (122 + (joystick_nb*5)) / 32] = (parmreg[4 + (122 + (joystick_nb*5)) / 32] & ~(1 << (122 + (joystick_nb*5)) % 32)) | (!event.value) << ((122 + (joystick_nb*5)) % 32);
                    break;

                case JS_EVENT_AXIS:
                    axis = get_axis_state(&event, axes);
                    if (axis < 3)
                    {
                        printf("Axis of joystick %d: %zu at (%6d, %6d)\n", joystick_nb, axis, axes[axis].x, axes[axis].y);
                        if (axes[axis].x < 0)
                        {
                            parmreg[4 + (120 + (joystick_nb*5)) / 32] = (parmreg[4 + (120 + (joystick_nb*5)) / 32] & ~(3 << (120 + (joystick_nb*5)) % 32)) | 2 << ((120 + (joystick_nb*5)) % 32);
                        }
                        if (axes[axis].x > 0)
                        {
                            parmreg[4 + (120 + (joystick_nb*5)) / 32] = (parmreg[4 + (120 + (joystick_nb*5)) / 32] & ~(3 << (120 + (joystick_nb*5)) % 32)) | 1 << ((120 + (joystick_nb*5)) % 32);
                        }
                        if (axes[axis].y < 0)
                        {
                            parmreg[4 + (118 + (joystick_nb*5)) / 32] = (parmreg[4 + (118 + (joystick_nb*5)) / 32] & ~(3 << (118 + (joystick_nb*5)) % 32)) | 2 << ((118 + (joystick_nb*5)) % 32);
                        }
                        if (axes[axis].y > 0)
                        {
                            parmreg[4 + (118 + (joystick_nb*5)) / 32] = (parmreg[4 + (118 + (joystick_nb*5)) / 32] & ~(3 << (118 + (joystick_nb*5)) % 32)) | 1 << ((118 + (joystick_nb*5)) % 32);
                        }
                        if (axes[axis].x == 0)
                        {
                            parmreg[4 + (120 + (joystick_nb*5)) / 32] = (parmreg[4 + (120 + (joystick_nb*5)) / 32] & ~(3 << (120 + (joystick_nb*5)) % 32)) | 3 << ((120 + (joystick_nb*5)) % 32);
                        }
                        if (axes[axis].y == 0)
                        {
                            parmreg[4 + (118 + (joystick_nb*5)) / 32] = (parmreg[4 + (118 + (joystick_nb*5)) / 32] & ~(3 << (118 + (joystick_nb*5)) % 32)) | 3 << ((118 + (joystick_nb*5)) % 32);
                        }
                    }

                    break;
                default:
                    /* Ignore init events. */
                    break;
            }

        }

        fflush(stdout);
    }

    printf("Joysticks unplugged!\n");
    close(js);

    return NULL;
}