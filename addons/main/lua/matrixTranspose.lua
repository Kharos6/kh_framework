local matrix = ...
local result = {}
local rows = #matrix
local cols = #matrix[1]

for j = 1, cols do
    result[j] = {}

    for i = 1, rows do
        result[j][i] = matrix[i][j]
    end
end

return result