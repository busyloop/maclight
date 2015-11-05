# -*- encoding: utf-8 -*-
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'maclight/version'

Gem::Specification.new do |gem|
  gem.name          = "maclight"
  gem.version       = MacLight::VERSION
  gem.authors       = ["Moe"]
  gem.email         = ["moe@busyloop.net"]
  gem.description   = %q{Control your Mac keyboard LEDs (capslock, numlock)}
  gem.summary       = %q{Control your Mac keyboard LEDs (capslock, numlock)}
  gem.homepage      = "https://github.com/busyloop/maclight"

  gem.files         = `git ls-files`.split($/)
  gem.executables   = gem.files.grep(%r{^bin/}).map{ |f| File.basename(f) }
  gem.test_files    = gem.files.grep(%r{^(test|spec|features)/})
  gem.require_paths = ["lib"]
  gem.extensions    = ['ext/maclight/extconf.rb']

  gem.add_dependency 'optix', '>= 1.2.2'
  gem.add_development_dependency 'rake', '~> 10.1.0'
end
