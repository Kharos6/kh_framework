local rangeFrom, rangeTo, rangeValue, vectorFrom, vectorTo, clip = ...
local t = (rangeValue - rangeFrom) / (rangeTo - rangeFrom)

if clip then
    if t < 0 then
        t = 0
    elseif t > 1 then
        t = 1
    end
end

local result = {}

for i = 1, #vectorFrom do
    result[i] = vectorFrom[i] + t * (vectorTo[i] - vectorFrom[i])
end

return result