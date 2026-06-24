local a, b = ...
local dot = 0
local sumA = 0
local sumB = 0

for i = 1, #a do
    dot = dot + a[i] * b[i]
    sumA = sumA + a[i] * a[i]
    sumB = sumB + b[i] * b[i]
end

return dot / (math.sqrt(sumA) * math.sqrt(sumB))