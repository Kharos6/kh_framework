local array = ...
local result = nil

for i, value in pairs(array) do
    if type(value) == "number" and (result == nil or value < result) then
        result = value
    end
end

return result