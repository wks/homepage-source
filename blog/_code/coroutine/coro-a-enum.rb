def traverse(x, &block)
  if x.is_a? Array
    x.each do |elem|
      traverse(elem, &block)
    end
  else
    block.yield(x)  # This is just a usual method call, not a coroutine yield.
  end
end

def enum_traverse(x)
  Enumerator.new do |yielder|   # The yielder encapsulates how to yield.
    traverse(x) do |value|
      yielder.yield(value)      # This may or may not use coroutine yield.
    end
  end
end

DATA = [1, [[2, 3], [4, 5]], [6, 7, 8]]

# We can do internal iteration
enum_traverse(DATA).each do |value|   # use call-back
  puts value
end

# and external iteration too.
e = enum_traverse(DATA)
loop do           # Break if StopIteration is raised.
  value = e.next  # This will use fiber.
  puts value
end
