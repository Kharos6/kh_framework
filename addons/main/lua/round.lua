local value, decimals = ...
local multiplier = 10 ^ (decimals or 0)
local scaled = value * multiplier

if scaled >= 0 then
    return math.floor(scaled + 0.5) / multiplier
end

return math.ceil(scaled - 0.5) / multiplier