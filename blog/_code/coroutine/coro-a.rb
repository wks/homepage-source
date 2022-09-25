def traverse(x)
  if x.is_a? Array
    x.each do |elem|
      traverse(elem)      # recursive call
    end
  else
    Fiber.yield x         # can yield within recursive calls
  end
end

def fiber_traverse(x)
  Fiber::new do
    traverse(x)
    raise StopIteration
  end
end

DATA = [1, [[2, 3], [4, 5]], [6, 7, 8]]

fiber = fiber_traverse DATA

loop do   # Break if StopIteration is raised.
  value = fiber.resume
  puts value
end
