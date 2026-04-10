local directionFormat, directionIncludes = ...

local function isPressed(action)
    return sqf.inputAction(action) ~= 0
end

local axisX = isPressed("TurnLeft") and "LEFTWARD" or isPressed("TurnRight") and "RIGHTWARD" or ""
local axisY = isPressed("MoveForward") and "FORWARD" or isPressed("MoveBack") and "BACKWARD" or ""
local axisZ = isPressed("MoveUp") and "UPWARD" or isPressed("MoveDown") and "DOWNWARD" or ""
local tilt = isPressed("EvasiveLeft") and "LEFT" or isPressed("EvasiveRight") and "RIGHT" or ""

local function filterByIncludes(axes)
    local out = {}

    for i, v in ipairs(axes) do
        if directionIncludes[i] then
            out[#out + 1] = v
        end
    end

    return out
end

if directionFormat == "BOOLEAN" then
    return {filterByIncludes({ axisX ~= "", axisY ~= "", axisZ ~= "" }), tilt}
elseif directionFormat == "ARMA_STRING" then
    local filtered = filterByIncludes({axisX, axisY, axisZ})
    local initials = {}

    for _, v in ipairs(filtered) do
        if v ~= "" then
            initials[#initials + 1] = v:sub(1, 1)
        end
    end

    return {table.concat(initials, "_"), tilt}
elseif directionFormat == "KH_STRING" then
    local filtered = filterByIncludes({axisY, axisX, axisZ})
    local parts = {}

    for _, v in ipairs(filtered) do
        if v ~= "" then
            parts[#parts + 1] = v
        end
    end

    return {table.concat(parts, "_"), tilt}
else
    return {filterByIncludes({axisX, axisY, axisZ}), tilt}
end