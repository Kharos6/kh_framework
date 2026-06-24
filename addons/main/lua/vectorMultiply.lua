local vector, scalar = ...
local result = {}

for i = 1, #vector do
    result[i] = vector[i] * scalar
end

return result