# MacLight [![Dependency Status](https://gemnasium.com/busyloop/maclight.png)](https://gemnasium.com/busyloop/maclight)

MacLight lets you control the keyboard LEDs on your Mac or Macbook.

## Screenshot

![capslock](https://github.com/busyloop/maclight/raw/master/ass/screenshot_capslock.jpg?raw=true)

## Installation

    $ gem install maclight

## Usage

```
$ maclight --help

Usage: maclight <command>

MacLight v3.0.0 - LED control utility

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

# Turn LEDs on
MacLight.all_leds(true)

sleep 2

# Turn LEDs off
MacLight.all_leds(false)
```

## Notice

MacLight can currently only toggle all LEDs at once
and has only been tested on OSX 10.11.3 (El Capitan).

