local a, b = ...
local diff = {}
local sum = 0

for i = 1, #a do
    local d = b[i] - a[i]
    diff[i] = d
    sum = sum + d * d
end

local magnitude = math.sqrt(sum)

if magnitude == 0 then
    return diff
end

local result = {}

for i = 1, #diff do
    result[i] = diff[i] / magnitude
end

return result