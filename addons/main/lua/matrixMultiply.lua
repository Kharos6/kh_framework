local a, b = ...
local result = {}
local rowsA = #a
local rowsB = #b
local colsB = #b[1]

for i = 1, rowsA do
    result[i] = {}

    for j = 1, colsB do
        local sum = 0

        for k = 1, rowsB do
            sum = sum + a[i][k] * b[k][j]
        end

        result[i][j] = sum
    end
end

return result