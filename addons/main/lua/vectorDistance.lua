local a, b = ...
local sum = 0

for i = 1, #a do
    local d = a[i] - b[i]
    sum = sum + d * d
end

return math.sqrt(sum)