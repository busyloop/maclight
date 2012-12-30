/*
 * keyboard_leds.c
 * Manipulate keyboard LEDs (capslock and numlock) programmatically.
 *
 * gcc -Wall -o keyboard_leds keyboard_leds.c -framework IOKit
 *     -framework CoreFoundation
 *
 * Copyright (c) 2007,2008 Amit Singh. All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *     
 *  THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 */

/* rubyfied by moe@busyloop.net */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <sysexits.h>
#include <mach/mach_error.h>

#include <IOKit/IOCFPlugIn.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDUsageTables.h>

#include "ruby.h"

static IOHIDElementCookie capslock_cookie = (IOHIDElementCookie)0;
static IOHIDElementCookie numlock_cookie  = (IOHIDElementCookie)0;
static int capslock_value = -1;
static int numlock_value  = -1;

void         usage(void);
inline void  print_errmsg_if_io_err(int expr, char* msg);
inline void  print_errmsg_if_err(int expr, char* msg);

io_service_t find_a_keyboard(void);
void         find_led_cookies(IOHIDDeviceInterface122** handle);
void         create_hid_interface(io_object_t hidDevice,
                                  IOHIDDeviceInterface*** hdi);
int          manipulate_led(UInt32 whichLED, SInt32 value);

inline void
print_errmsg_if_io_err(int expr, char* msg)
{
    IOReturn err = (expr);

    if (err != kIOReturnSuccess) {
        fprintf(stderr, "*** %s - %s(%x, %d).\n", msg, mach_error_string(err),
                err, err & 0xffffff);
        fflush(stderr);
        exit(EX_OSERR);
    }
}

inline void
print_errmsg_if_err(int expr, char* msg)
{
    if (expr) {
        fprintf(stderr, "*** %s.\n", msg);
        fflush(stderr);
        exit(EX_OSERR);
    }
}

io_service_t
find_a_keyboard(void)
{
    io_service_t result = (io_service_t)0;

    CFNumberRef usagePageRef = (CFNumberRef)0;
    CFNumberRef usageRef = (CFNumberRef)0;
    CFMutableDictionaryRef matchingDictRef = (CFMutableDictionaryRef)0;

    if (!(matchingDictRef = IOServiceMatching(kIOHIDDeviceKey))) {
        return result;
    }

    UInt32 usagePage = kHIDPage_GenericDesktop;
    UInt32 usage = kHIDUsage_GD_Keyboard;

    if (!(usagePageRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType,
                                        &usagePage))) {
        goto out;
    }

    if (!(usageRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType,
                                    &usage))) {
        goto out;
    }

    CFDictionarySetValue(matchingDictRef, CFSTR(kIOHIDPrimaryUsagePageKey),
                         usagePageRef);
    CFDictionarySetValue(matchingDictRef, CFSTR(kIOHIDPrimaryUsageKey),
                         usageRef);

    result = IOServiceGetMatchingService(kIOMasterPortDefault, matchingDictRef);

out:
    if (usageRef) {
        CFRelease(usageRef);
    }
    if (usagePageRef) {
        CFRelease(usagePageRef);
    }

    return result;
}

void
find_led_cookies(IOHIDDeviceInterface122** handle)
{
    IOHIDElementCookie cookie;
    CFTypeRef          object;
    long               number;
    long               usage;
    long               usagePage;
    CFArrayRef         elements;
    CFDictionaryRef    element;
    IOReturn           result;

    if (!handle || !(*handle)) {
        return;
    }

    result = (*handle)->copyMatchingElements(handle, NULL, &elements);

    if (result != kIOReturnSuccess) {
        fprintf(stderr, "Failed to copy cookies.\n");
        exit(1);
    }

    CFIndex i;

    for (i = 0; i < CFArrayGetCount(elements); i++) {

        element = CFArrayGetValueAtIndex(elements, i);

        object = (CFDictionaryGetValue(element, CFSTR(kIOHIDElementCookieKey)));
        if (object == 0 || CFGetTypeID(object) != CFNumberGetTypeID()) {
            continue;
        }
        if (!CFNumberGetValue((CFNumberRef) object, kCFNumberLongType,
                              &number)) {
            continue;
        }
        cookie = (IOHIDElementCookie)number;

        object = CFDictionaryGetValue(element, CFSTR(kIOHIDElementUsageKey));
        if (object == 0 || CFGetTypeID(object) != CFNumberGetTypeID()) {
            continue;
        }
        if (!CFNumberGetValue((CFNumberRef)object, kCFNumberLongType,
                              &number)) {
            continue;
        }
        usage = number;

        object = CFDictionaryGetValue(element,CFSTR(kIOHIDElementUsagePageKey));
        if (object == 0 || CFGetTypeID(object) != CFNumberGetTypeID()) {
            continue;
        }
        if (!CFNumberGetValue((CFNumberRef)object, kCFNumberLongType,
                              &number)) {
            continue;
        }
        usagePage = number;

        if (usagePage == kHIDPage_LEDs) {
            switch (usage) {

            case kHIDUsage_LED_NumLock:
                numlock_cookie = cookie;
                break;

            case kHIDUsage_LED_CapsLock:
                capslock_cookie = cookie;
                break;

            default:
                break;
            }
        }
    }

    return;
}

void
create_hid_interface(io_object_t hidDevice, IOHIDDeviceInterface*** hdi)
{
    IOCFPlugInInterface** plugInInterface = NULL;

    io_name_t className;
    HRESULT   plugInResult = S_OK;
    SInt32    score = 0;
    IOReturn  ioReturnValue = kIOReturnSuccess;

    ioReturnValue = IOObjectGetClass(hidDevice, className);

    //print_errmsg_if_io_err(ioReturnValue, "Failed to get class name.");

    ioReturnValue = IOCreatePlugInInterfaceForService(
                        hidDevice, kIOHIDDeviceUserClientTypeID,
                        kIOCFPlugInInterfaceID, &plugInInterface, &score);
    if (ioReturnValue != kIOReturnSuccess) {
        return;
    }

    plugInResult = (*plugInInterface)->QueryInterface(plugInInterface,
                     CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID), (LPVOID)hdi);
    //print_errmsg_if_err(plugInResult != S_OK,
    //                    "Failed to create device interface.\n");

    (*plugInInterface)->Release(plugInInterface);
}

int
manipulate_led(UInt32 whichLED, SInt32 value)
{
    io_service_t           hidService = (io_service_t)0;
    io_object_t            hidDevice = (io_object_t)0;
    IOHIDDeviceInterface **hidDeviceInterface = NULL;
    IOReturn               ioReturnValue = kIOReturnError;
    IOHIDElementCookie     theCookie = (IOHIDElementCookie)0;
    IOHIDEventStruct       theEvent;

    if (!(hidService = find_a_keyboard())) {
        fprintf(stderr, "No keyboard found.\n");
        return ioReturnValue;
    }

    hidDevice = (io_object_t)hidService;

    create_hid_interface(hidDevice, &hidDeviceInterface);

    find_led_cookies((IOHIDDeviceInterface122 **)hidDeviceInterface);

    ioReturnValue = IOObjectRelease(hidDevice);
    if (ioReturnValue != kIOReturnSuccess) {
        goto out;
    }

    ioReturnValue = kIOReturnError;

    if (hidDeviceInterface == NULL) {
        fprintf(stderr, "Failed to create HID device interface.\n");
        return ioReturnValue;
    }

    if (whichLED == kHIDUsage_LED_NumLock) {
        theCookie = numlock_cookie;
    } else if (whichLED == kHIDUsage_LED_CapsLock) {
        theCookie = capslock_cookie;
    }

    if (theCookie == 0) {
        fprintf(stderr, "Bad or missing LED cookie.\n");
        goto out;
    }

    ioReturnValue = (*hidDeviceInterface)->open(hidDeviceInterface, 0);
    if (ioReturnValue != kIOReturnSuccess) {
        fprintf(stderr, "Failed to open HID device interface.\n");
        goto out;
    }

    ioReturnValue = (*hidDeviceInterface)->getElementValue(hidDeviceInterface,
                                               theCookie, &theEvent);
    if (ioReturnValue != kIOReturnSuccess) {
        (void)(*hidDeviceInterface)->close(hidDeviceInterface);
        goto out;
    }

    //fprintf(stdout, "%s\n", (theEvent.value) ? "on" : "off");
    if (value != -1) {
        if (theEvent.value != value) {
            theEvent.value = value;
            ioReturnValue = (*hidDeviceInterface)->setElementValue(
                                hidDeviceInterface, theCookie,
                                &theEvent, 0, 0, 0, 0);
            //if (ioReturnValue == kIOReturnSuccess) {
            //    fprintf(stdout, "%s\n", (theEvent.value) ? "on" : "off");
            //}
        }
    }

    ioReturnValue = (*hidDeviceInterface)->close(hidDeviceInterface);

out:
    (void)(*hidDeviceInterface)->Release(hidDeviceInterface);

    //return ioReturnValue;
    return theEvent.value;
}

VALUE MacLight = Qnil;

void Init_maclight();

VALUE method_capslock(int argc, VALUE *argv, VALUE self);
VALUE  method_numlock(int argc, VALUE *argv, VALUE self);

void Init_maclight() {
  MacLight = rb_define_module("MacLight");
  rb_define_singleton_method(MacLight, "capslock", method_capslock, -1);
  rb_define_singleton_method(MacLight, "numlock", method_numlock, -1);
}

VALUE kbd_led(UInt32 whichLED, int argc, VALUE *argv, VALUE klass) {
  VALUE flag;
  rb_scan_args(argc, argv, "01", &flag);

  int set_to = -1;
  switch (flag) {
    case Qtrue:
      set_to = 1;
      break;
    case Qfalse:
      set_to = 0;
      break;
  }

  return manipulate_led(whichLED, set_to) ? Qtrue : Qfalse;
}

VALUE method_capslock(int argc, VALUE *argv, VALUE klass) {
  return kbd_led(kHIDUsage_LED_CapsLock, argc, argv, klass);
}

VALUE method_numlock(int argc, VALUE *argv, VALUE klass) {
  return kbd_led(kHIDUsage_LED_NumLock, argc, argv, klass);
}

