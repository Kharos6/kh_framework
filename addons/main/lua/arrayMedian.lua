local array = ...
local values = {}
local count = 0

for i, value in pairs(array) do
    if type(value) == "number" then
        count = count + 1
        values[count] = value
    end
end

if count == 0 then
    return nil
end

table.sort(values)

if count % 2 == 1 then
    return values[(count + 1) / 2]
end

local mid = count / 2

return (values[mid] + values[mid + 1]) / 2