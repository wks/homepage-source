function traverse(x)
  if type(x) == "table" then
    for i, v in ipairs(x) do
      traverse(v)
    end
  else
    coroutine.yield(x)
  end
end

function coroutine_traverse(x)
  return coroutine.wrap(function()
    traverse(x)
  end)
end

local list = {1, {{2, 3}, {4, 5}}, {6, 7, 8}}

for value in coroutine_traverse(list) do
  print(value)
end
