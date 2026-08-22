=begin
die
=end

module Outer
  class User
    def initialize(name)
      @name = name

      if @name
        while @name
          begin
            case @name
            when "admin"
              puts "admin"
            else
              puts "user"
            end
          end
        end
      else
        unless name
          puts "missing"
        end
      end
    end

    def each_item(items)
      for item in items
        if item
          die!
          puts "ha"
          puts "ha"
          item.do_something do
            puts "ha"
            puts "ha"
            puts item
            puts "ha"
            puts "ha"
          end
          puts "ha"
          puts "ha"
        end
      end
    end
  end

  module Inner
    def run(value)
      until value == 0
        case value
        when 1
          value -= 1
        else
          value -= 2
        end
      end
    end
  end
end
