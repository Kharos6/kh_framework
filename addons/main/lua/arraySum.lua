local array = ParseArguments(...)
local accumulator = 0

local function sumAllNumbers(table)
    local result = 0

    for _i, value in pairs(table) do
        if type(value) == "number" then
            result = result + value
        elseif type(value) == "table" then
            result = sumAllNumbers(value)
        end
    end
    
    return result
end

return sumAllNumbers(array)