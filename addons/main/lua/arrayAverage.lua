local array = ...
local sum = 0
local count = 0

for i, value in pairs(array) do
    if type(value) == "number" then
        sum = sum + value
        count = count + 1
    end
end

if count == 0 then
    return nil
end

return sum / count