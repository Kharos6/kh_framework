local array = ...
local result = 1

for i, value in pairs(array) do
    if type(value) == "number" then
        result = result * value
    elseif type(value) == "table" then
        result = result * KH_arrayProduct(value)
    end
end

return result