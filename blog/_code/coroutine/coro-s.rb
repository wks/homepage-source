require "fiber" 

def traverse(x, parent)
  if x.is_a? Array
    x.each do |elem|
      traverse(elem, parent)  # always remember the parent
    end
  else
    parent.transfer x
  end
end

def fiber_traverse(x)
  current = Fiber.current     # get the current fiber
  Fiber.new do
    traverse(x, current)      # pass the fiber as parent
    raise StopIteration
  end
end

DATA = [1, [[2, 3], [4, 5]], [6, 7, 8]]

fiber = fiber_traverse DATA
loop do   # Break if StopIteration is raised.
  value = fiber.transfer
  puts value
end
