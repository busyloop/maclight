require 'maclight'
require 'maclight/version'
require 'optix'

module MacLight
  class Cli < Optix::Cli

    cli_root do
      text "MacLight v#{MacLight::VERSION} - LED control utility"

      opt :version, "Print version and exit", :short => :none
      trigger :version do
        puts "MacLight v#{MacLight::VERSION}"
      end
    end

    desc "Toggle keyboard LEDs"
    text "Toggle keyboard LEDs (capslock, numlock)"
    opt :capslock, "Toggle capslock LED", :default => false
    opt :numlock, "Toggle numlock LED", :default => false
    opt :verbose, "Print current state of capslock, numlock"
    parent "keyboard", "Control keyboard LEDs"
    def toggle(cmd, opts, argv)
      raise Optix::HelpNeeded unless opts.values_at(:capslock, :numlock, :verbose).any?
      MacLight.capslock(!MacLight.capslock) if opts[:capslock]
      MacLight.numlock(!MacLight.numlock) if opts[:numlock]
      puts "#{MacLight.capslock ? 1:0} #{MacLight.numlock ? 1:0}" if opts[:verbose]
    end

    desc "Set keyboard LEDs state"
    text "Set keyboard LEDs (capslock, numlock) state (on, off)"
    opt :capslock, "Set capslock LED (0|1)", :type => Integer
    opt :numlock, "Set numlock LED (0|1)", :type => Integer
    opt :verbose, "Print current state of capslock, numlock"
    parent "keyboard"
    def set(cmd, opts, argv)
      raise Optix::HelpNeeded unless opts.values_at(:capslock, :numlock, :verbose).any?
      MacLight.capslock(1 == opts[:capslock]) unless opts[:capslock].nil?
      MacLight.numlock(1 == opts[:numlock]) unless opts[:numlock].nil?
      puts "#{MacLight.capslock ? 1:0} #{MacLight.numlock ? 1:0}" if opts[:verbose]
    end

    desc "Blink keyboard LEDs"
    text "Blink keyboard LEDs (capslock, numlock)"
    text ''
    text "Examples:"
    text "  #{File.basename($0)} keyboard blink -r 3 -f 00 10:0.3 01:0.3"
    text "  #{File.basename($0)} keyboard blink -r 3 10:0.3 01:0.2 00:0.1 11:0.2 00:0.07 11:0.07 00:0.07"
    text ''
    text 'Parameters:'
    text '  <sequence> - Space-delimited sequence; CN:T CN:T ..'
    text '               C = capslock value, N = numlock value, T = time in seconds'
    opt :repeat, "Repetitions", :default => 0
    opt :fin, "Set this state after sequence has finished (CN)", :type => String
    opt :verbose, "Print state of capslock, numlock"
    params "<sequence>"
    parent "keyboard"
    def blink(cmd, opts, argv)
      raise Optix::HelpNeeded if argv.empty?
      puts "#{MacLight.capslock ? 1:0} #{MacLight.numlock ? 1:0}" if opts[:verbose]
      seq = argv.map {|e| ['1'==e[0],'1'==e[1],e[3..-1].to_f] }
      (0..opts[:repeat]).each do |i|
        seq.each_with_index do |mode, step|
          MacLight.capslock(mode[0])
          MacLight.numlock(mode[1])
          puts "#{MacLight.capslock ? 1:0} #{MacLight.numlock ? 1:0}" if opts[:verbose]
          sleep mode[2]
        end
      end
      if opts[:fin]
        MacLight.capslock('1'==opts[:fin][0])
        MacLight.numlock('1'==opts[:fin][1])
        puts "#{MacLight.capslock ? 1:0} #{MacLight.numlock ? 1:0}" if opts[:verbose]
      end
    end

    def self.start
      Optix.invoke!
    end
  end
end
