local a, b = ...
local result = {}

for i = 1, #a do
    result[i] = a[i] - b[i]
end

return result