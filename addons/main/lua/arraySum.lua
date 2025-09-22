local inputArray = ...
inputArray = inputArray or {}
local accumulator = 0

local function sumAllNumbers(table)    
    for _i, value in pairs(table) do
        if type(value) == "number" then
            accumulator = accumulator + value
        elseif type(value) == "table" then
            accumulator = sumAllNumbers(value)
        end
    end
    
    return accumulator
end

return sumAllNumbers(inputArray)