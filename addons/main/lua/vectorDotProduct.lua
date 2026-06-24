local a, b = ...
local result = 0

for i = 1, #a do
    result = result + a[i] * b[i]
end

return result