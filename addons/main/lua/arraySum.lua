local array = ...
local result = 0

for i, value in pairs(array) do
    if type(value) == "number" then
        result = result + value
    elseif type(value) == "table" then
        result = KH_arraySum(value)
    end
end

return result