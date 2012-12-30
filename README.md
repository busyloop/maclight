# MacLight [![Dependency Status](https://gemnasium.com/busyloop/maclight.png)](https://gemnasium.com/busyloop/maclight)

MacLight lets you control the keyboard-LEDs (capslock, numlock) on your Mac.

## Screenshot

![capslock](http://github.com/busyloop/maclight/raw/master/ass/screenshot_capslock.jpg)

## Installation

    $ gem install maclight

## Usage

```bash
$ maclight --help

Usage: maclight <subcommand>

MacLight v1.0.0 - LED control utility
 
Options:
   --version:   Print version and exit
  --help, -h:   Show this message

Commands:
   keyboard   Control keyboard LEDs
```


## API Usage

```ruby
#!/usr/bin/env ruby

require 'maclight'

# Turn both LEDs on
MacLight.capslock(true)
MacLight.numlock(true)

sleep 2

# Turn both LEDs off
MacLight.capslock(false)
MacLight.numlock(false)
```

## Credits

MacLight is based on HID demonstration code by [Amit Singh](http://googlemac.blogspot.de/2008/04/manipulating-keyboard-leds-through.html).
