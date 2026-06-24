local rangeFrom, rangeTo, value, newFrom, newTo, clip = ...
local t = (value - rangeFrom) / (rangeTo - rangeFrom)

if clip then
    if t < 0 then
        t = 0
    elseif t > 1 then
        t = 1
    end
end

return newFrom + t * (newTo - newFrom)