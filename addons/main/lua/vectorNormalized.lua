local vector = ...
local sum = 0

for i = 1, #vector do
    sum = sum + vector[i] * vector[i]
end

local magnitude = math.sqrt(sum)
local result = {}

if magnitude == 0 then
    for i = 1, #vector do
        result[i] = 0
    end

    return result
end

for i = 1, #vector do
    result[i] = vector[i] / magnitude
end

return result