//     File: main.c
// Abstract: source code for HID LED test tool
//  Version: 1.3
//
// Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
// Inc. ("Apple") in consideration of your agreement to the following
// terms, and your use, installation, modification or redistribution of
// this Apple software constitutes acceptance of these terms.  If you do
// not agree with these terms, please do not use, install, modify or
// redistribute this Apple software.
//
// In consideration of your agreement to abide by the following terms, and
// subject to these terms, Apple grants you a personal, non-exclusive
// license, under Apple's copyrights in this original Apple software (the
// "Apple Software"), to use, reproduce, modify and redistribute the Apple
// Software, with or without modifications, in source and/or binary forms;
// provided that if you redistribute the Apple Software in its entirety and
// without modifications, you must retain this notice and the following
// text and disclaimers in all such redistributions of the Apple Software.
// Neither the name, trademarks, service marks or logos of Apple Inc. may
// be used to endorse or promote products derived from the Apple Software
// without specific prior written permission from Apple.  Except as
// expressly stated in this notice, no other rights or licenses, express or
// implied, are granted by Apple herein, including but not limited to any
// patent rights that may be infringed by your derivative works or by other
// works in which the Apple Software may be incorporated.
//
// The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
// MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
// THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
// OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
//
// IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
// OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
// MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
// AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
// STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (C) 2015 Apple Inc. All Rights Reserved.
//
// ****************************************************

/* rubyfied by moe@busyloop.net */
#pragma mark -
#pragma mark * complation directives *
// ----------------------------------------------------
/* rubyfied by moe@busyloop.net */

#ifndef FALSE
#define FALSE 0
#define TRUE !FALSE
#endif

// ****************************************************
#pragma mark -
#pragma mark * includes & imports *
// ----------------------------------------------------

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDLib.h>

#include "ruby.h"

// ****************************************************
#pragma mark -
#pragma mark * typedef's, struct's, enums, defines, etc. *
// ----------------------------------------------------
// function to create a matching dictionary for usage page & usage
static CFMutableDictionaryRef hu_CreateMatchingDictionaryUsagePageUsage( Boolean isDeviceNotElement,
																		UInt32 inUsagePage,
																		UInt32 inUsage )
{
	// create a dictionary to add usage page / usages to
	CFMutableDictionaryRef result = CFDictionaryCreateMutable( kCFAllocatorDefault,
															  0,
															  &kCFTypeDictionaryKeyCallBacks,
															  &kCFTypeDictionaryValueCallBacks );

	if ( result ) {
		if ( inUsagePage ) {
			// Add key for device type to refine the matching dictionary.
			CFNumberRef pageCFNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &inUsagePage );

			if ( pageCFNumberRef ) {
				if ( isDeviceNotElement ) {
					CFDictionarySetValue( result, CFSTR( kIOHIDDeviceUsagePageKey ), pageCFNumberRef );
				} else {
					CFDictionarySetValue( result, CFSTR( kIOHIDElementUsagePageKey ), pageCFNumberRef );
				}
				CFRelease( pageCFNumberRef );

				// note: the usage is only valid if the usage page is also defined
				if ( inUsage ) {
					CFNumberRef usageCFNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &inUsage );

					if ( usageCFNumberRef ) {
						if ( isDeviceNotElement ) {
							CFDictionarySetValue( result, CFSTR( kIOHIDDeviceUsageKey ), usageCFNumberRef );
						} else {
							CFDictionarySetValue( result, CFSTR( kIOHIDElementUsageKey ), usageCFNumberRef );
						}
						CFRelease( usageCFNumberRef );
					} else {
						fprintf( stderr, "%s: CFNumberCreate( usage ) failed.", __PRETTY_FUNCTION__ );
					}
				}
			} else {
				fprintf( stderr, "%s: CFNumberCreate( usage page ) failed.", __PRETTY_FUNCTION__ );
			}
		}
	} else {
		fprintf( stderr, "%s: CFDictionaryCreateMutable failed.", __PRETTY_FUNCTION__ );
	}
	return result;
}	// hu_CreateMatchingDictionaryUsagePageUsage

//int main( int argc, const char * argv[] )
int manipulate_led( int which_led, int led_value )
{
#pragma unused ( argc, argv )
	IOHIDDeviceRef * tIOHIDDeviceRefs = nil;

	// create a IO HID Manager reference
	IOHIDManagerRef tIOHIDManagerRef = IOHIDManagerCreate( kCFAllocatorDefault, kIOHIDOptionsTypeNone );
	__Require( tIOHIDManagerRef, Oops );

	// Create a device matching dictionary
	CFDictionaryRef matchingCFDictRef = hu_CreateMatchingDictionaryUsagePageUsage( TRUE,
																				  kHIDPage_GenericDesktop,
																				  kHIDUsage_GD_Keyboard );
	__Require( matchingCFDictRef, Oops );

	// set the HID device matching dictionary
	IOHIDManagerSetDeviceMatching( tIOHIDManagerRef, matchingCFDictRef );

	if ( matchingCFDictRef ) {
		CFRelease( matchingCFDictRef );
	}

	// Now open the IO HID Manager reference
	IOReturn tIOReturn = IOHIDManagerOpen( tIOHIDManagerRef, kIOHIDOptionsTypeNone );
	__Require_noErr( tIOReturn, Oops );

	// and copy out its devices
	CFSetRef deviceCFSetRef = IOHIDManagerCopyDevices( tIOHIDManagerRef );
	__Require( deviceCFSetRef, Oops );

	// how many devices in the set?
	CFIndex deviceIndex, deviceCount = CFSetGetCount( deviceCFSetRef );

	// allocate a block of memory to extact the device ref's from the set into
	tIOHIDDeviceRefs = malloc( sizeof( IOHIDDeviceRef ) * deviceCount );
	if (!tIOHIDDeviceRefs) {
		CFRelease(deviceCFSetRef);
		deviceCFSetRef = NULL;
		goto Oops;
	}

	// now extract the device ref's from the set
	CFSetGetValues( deviceCFSetRef, (const void **) tIOHIDDeviceRefs );
	CFRelease(deviceCFSetRef);
	deviceCFSetRef = NULL;

	// before we get into the device loop we'll setup our element matching dictionary
	matchingCFDictRef = hu_CreateMatchingDictionaryUsagePageUsage( FALSE, kHIDPage_LEDs, 0 );
	__Require( matchingCFDictRef, Oops );

	int pass;	// do 256 passes
	//for ( pass = 0; pass < 256; pass++ ) {
		Boolean delayFlag = FALSE;	// if we find an LED element we'll set this to TRUE

		//printf( "pass = %d.\n", pass );
		for ( deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++ ) {

			// if this isn't a keyboard device...
			if ( !IOHIDDeviceConformsTo( tIOHIDDeviceRefs[deviceIndex], kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard ) ) {
				continue;	// ...skip it
			}

			//printf( "	 device = %p.\n", tIOHIDDeviceRefs[deviceIndex] );

			// copy all the elements
			CFArrayRef elementCFArrayRef = IOHIDDeviceCopyMatchingElements( tIOHIDDeviceRefs[deviceIndex],
																		   matchingCFDictRef,
																		   kIOHIDOptionsTypeNone );
			__Require( elementCFArrayRef, next_device );

			// for each device on the system these values are divided by the value ranges of all LED elements found
			// for example, if the first four LED element have a range of 0-1 then the four least significant bits of
			// this value will be sent to these first four LED elements, etc.
			int device_value = pass;

			// iterate over all the elements
			CFIndex elementIndex, elementCount = CFArrayGetCount( elementCFArrayRef );
			for ( elementIndex = 0; elementIndex < elementCount; elementIndex++ ) {
				IOHIDElementRef tIOHIDElementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( elementCFArrayRef, elementIndex );
				__Require( tIOHIDElementRef, next_element );

				uint32_t usagePage = IOHIDElementGetUsagePage( tIOHIDElementRef );

				// if this isn't an LED element...
				if ( kHIDPage_LEDs != usagePage ) {
					continue;	// ...skip it
				}

				uint32_t usage = IOHIDElementGetUsage( tIOHIDElementRef );
				IOHIDElementType tIOHIDElementType = IOHIDElementGetType( tIOHIDElementRef );

				//printf( "		 element = %p (page: %d, usage: %d, type: %d ).\n",
				//	   tIOHIDElementRef, usagePage, usage, tIOHIDElementType );

				// get the logical mix/max for this LED element
				CFIndex minCFIndex = IOHIDElementGetLogicalMin( tIOHIDElementRef );
				CFIndex maxCFIndex = IOHIDElementGetLogicalMax( tIOHIDElementRef );

				// calculate the range
				CFIndex modCFIndex = maxCFIndex - minCFIndex + 1;

				// compute the value for this LED element
				//CFIndex tCFIndex = minCFIndex + ( device_value % modCFIndex );
				CFIndex tCFIndex = led_value;
				device_value /= modCFIndex;

				//printf( "			 value = 0x%08lX.\n", tCFIndex );

				uint64_t timestamp = 0; // create the IO HID Value to be sent to this LED element
				IOHIDValueRef tIOHIDValueRef = IOHIDValueCreateWithIntegerValue( kCFAllocatorDefault, tIOHIDElementRef, timestamp, tCFIndex );
				if ( tIOHIDValueRef ) {
					// now set it on the device
					tIOReturn = IOHIDDeviceSetValue( tIOHIDDeviceRefs[deviceIndex], tIOHIDElementRef, tIOHIDValueRef );
					CFRelease( tIOHIDValueRef );
					__Require_noErr( tIOReturn, next_element );
					delayFlag = TRUE;	// set this TRUE so we'll delay before changing our LED values again
				}
			next_element:	;
				continue;
			}
			CFRelease( elementCFArrayRef );
		next_device: ;
			continue;
		}

		// if we found an LED we'll delay before continuing
		//if ( delayFlag ) {
		//	usleep( 500000 ); // sleep one half second
		//}

		// if the mouse is down…
		//if (GetCurrentButtonState()) {
		//	break;	// abort pass loop
		//}
	//}						  // next pass

	if ( matchingCFDictRef ) {
		CFRelease( matchingCFDictRef );
	}
Oops:	;
	if ( tIOHIDDeviceRefs ) {
		free(tIOHIDDeviceRefs);
	}

	if ( tIOHIDManagerRef ) {
		CFRelease( tIOHIDManagerRef );
	}
	return 0;
} /* main */

VALUE MacLight = Qnil;

void Init_maclight();

//VALUE method_capslock(int argc, VALUE *argv, VALUE self);
//VALUE  method_numlock(int argc, VALUE *argv, VALUE self);
VALUE  method_all_leds(int argc, VALUE *argv, VALUE self);

void Init_maclight() {
  MacLight = rb_define_module("MacLight");
  //rb_define_singleton_method(MacLight, "capslock", method_capslock, -1);
  //rb_define_singleton_method(MacLight, "numlock", method_numlock, -1);
  rb_define_singleton_method(MacLight, "all_leds", method_all_leds, -1);
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

VALUE method_all_leds(int argc, VALUE *argv, VALUE klass) {
  return kbd_led(-1, argc, argv, klass);
}

//VALUE method_capslock(int argc, VALUE *argv, VALUE klass) {
//  return kbd_led(kHIDUsage_LED_CapsLock, argc, argv, klass);
//}
//
//VALUE method_numlock(int argc, VALUE *argv, VALUE klass) {
//  return kbd_led(kHIDUsage_LED_NumLock, argc, argv, klass);
//}

