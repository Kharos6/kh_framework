local array, element = ...
local result = 0

for i, value in pairs(array) do
    if value == element then
        result = result + 1
    end
end

return result