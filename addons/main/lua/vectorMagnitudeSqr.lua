local vector = ...
local sum = 0

for i = 1, #vector do
    sum = sum + vector[i] * vector[i]
end

return sum