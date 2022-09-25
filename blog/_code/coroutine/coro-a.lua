function traverse(x)
  if type(x) == "table" then
    for i, v in ipairs(x) do
      traverse(v)         -- recursive call
    end
  else
    coroutine.yield(x)    -- can yield within recursive calls
  end
end

function coroutine_traverse(x)
  return coroutine.create(function()
    traverse(x)
    return nil
  end)
end

local list = {1, {{2, 3}, {4, 5}}, {6, 7, 8}}

local coro = coroutine_traverse(list)

while true do
  local _, value = coroutine.resume(coro)
  if value == nil then
    break   -- terminated
  end
  print(value)
end
