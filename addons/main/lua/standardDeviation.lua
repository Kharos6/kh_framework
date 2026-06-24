local array = ...
local values = {}
local count = 0
local sum = 0

for i, value in pairs(array) do
    if type(value) == "number" then
        count = count + 1
        values[count] = value
        sum = sum + value
    end
end

if count == 0 then
    return nil
end

local mean = sum / count
local total = 0

for i = 1, count do
    local diff = values[i] - mean
    total = total + diff * diff
end

return math.sqrt(total / count)