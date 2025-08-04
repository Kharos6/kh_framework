local function factorial(n)
    if n <= 1 then
        return 1 
    end

    return n * factorial(n - 1)
end

local function fibonacci(n)
    if n <= 1 then
        return n 
    end

    return fibonacci(n - 1) + fibonacci(n - 2)
end

local function isPrime(n)
    if n < 2 then
        return false
    end

    for i = 2, math.sqrt(n) do
        if n % i == 0 then return false end
    end

    return true
end

local results = {}
results[1] = nil
results[2] = true
results[3] = false
results[4] = 0
results[5] = 1
results[6] = -1
results[7] = 42
results[8] = -42
results[9] = 3.14159
results[10] = -2.718
results[11] = 1.23e10
results[12] = 1.23e-10
results[13] = math.huge
results[14] = -math.huge
results[15] = ''
results[16] = 'hello'
results[17] = 'world'
results[18] = 'Hello, World!'
results[19] = 'Line1\nLine2\tTabbed'
results[20] = 'Special chars: !@#$%^&*()'
results[21] = 'Unicode: αβγδε'
results[22] = string.rep('x', 100)
results[23] = 'hello' .. '_' .. 'world'
results[24] = string.upper('lowercase')
results[25] = string.lower('UPPERCASE')
results[26] = string.sub('substring', 4, 6)
results[27] = string.len('length test')
results[28] = string.reverse('reverse')
results[29] = tostring(12345)
results[30] = tonumber('67890')
results[31] = 10 + 5
results[32] = 20 - 8
results[33] = 6 * 7
results[34] = 84 / 2
results[35] = 17 % 5
results[36] = 2 ^ 8
results[37] = math.abs(-15)
results[38] = math.floor(4.9)
results[39] = math.ceil(4.1)
results[40] = math.min(1, 2, 3)
results[41] = math.max(1, 2, 3)
results[42] = math.sqrt(16)
results[43] = math.sin(math.pi / 2)
results[44] = math.cos(0)
results[45] = math.log(20)
results[46] = true and true
results[47] = true and false
results[48] = false or true
results[49] = not true
results[50] = not false
results[51] = 5 > 3
results[52] = 5 < 3
results[53] = 5 >= 5
results[54] = 5 <= 5
results[55] = 5 == 5
results[56] = 5 ~= 3
results[57] = {}
results[58] = {1}
results[59] = {1, 2, 3}
results[60] = {1, 2, 3, 4, 5}
results[61] = {'a', 'b', 'c'}
results[62] = {true, false, nil}
results[63] = {1, 'two', 3.0, true, nil}
results[64] = {nested = {value = 42}}
results[65] = {{1, 2}, {3, 4}}
results[66] = {x = 10, y = 20, z = 30}
results[67] = {name = 'test', value = 123, active = true}
results[68] = {[1] = 'first', [2] = 'second', key = 'value'}

local temp_array = {}

for i = 1, 10 do
    temp_array[i] = i * i
end

results[69] = temp_array

local temp_sum = 0

for i = 1, 100 do
    temp_sum = temp_sum + i
end

results[70] = temp_sum

local temp_count = 0
local i = 1

while i <= 50 do
    if i % 2 == 0 then
        temp_count = temp_count + 1
    end

    i = i + 1
end

results[71] = temp_count
results[72] = factorial(5)
results[73] = factorial(0)
results[74] = factorial(10)
results[75] = fibonacci(0)
results[76] = fibonacci(1)
results[77] = fibonacci(10)

local primes = {}
local prime_count = 0

for i = 2, 50 do
    if isPrime(i) then
        prime_count = prime_count + 1
        primes[prime_count] = i
    end
end

results[78] = primes
results[79] = prime_count

local mixed_operations = {
    math.floor(math.random() * 1000),
    string.format('%.2f', 3.14159),
    (#'test string'),
    type(42),
    type('string'),
    type(true),
    type({}),
    type(nil)
}

results[80] = mixed_operations

if 5 > 3 then
    results[81] = 'condition true'
else
    results[81] = 'condition false'
end

local switch_test = 2
local switch_result

if switch_test == 1 then
    switch_result = 'one'
elseif switch_test == 2 then
    switch_result = 'two'
elseif switch_test == 3 then
    switch_result = 'three'
else
    switch_result = 'other'
end

results[82] = switch_result

local nested_loop_result = {}

for i = 1, 3 do
    nested_loop_result[i] = {}
    for j = 1, 3 do
        nested_loop_result[i][j] = i * j
    end
end

results[83] = nested_loop_result

local string_array = {'apple', 'banana', 'cherry', 'date'}
local concatenated = ''

for k, v in ipairs(string_array) do
    concatenated = concatenated .. v

    if k < #string_array then
        concatenated = concatenated .. ', '
    end
end

results[84] = concatenated

local key_value_pairs = {a = 1, b = 2, c = 3, d = 4}
local keys = {}
local values = {}
local pair_count = 0

for k, v in pairs(key_value_pairs) do
    pair_count = pair_count + 1
    keys[pair_count] = k
    values[pair_count] = v
end

results[85] = keys
results[86] = values
results[87] = string.match('abc123def', '%d+')
results[88] = string.gsub('hello world', 'world', 'lua')
results[89] = string.find('testing', 'st')

local complex_table = {
    metadata = {
        version = '1.0',
        author = 'test',
        created = 1234567890
    },

    data = {
        {id = 1, name = 'item1', value = 100},
        {id = 2, name = 'item2', value = 200},
        {id = 3, name = 'item3', value = 300}
    },

    settings = {
        enabled = true,
        threshold = 0.75,
        options = {'opt1', 'opt2', 'opt3'}
    }
}

results[90] = complex_table

local matrix = {}

for i = 1, 3 do
    matrix[i] = {}

    for j = 1, 3 do
        matrix[i][j] = i + j
    end
end

results[91] = matrix

local type_conversion_tests = {
    tonumber('123'),
    tonumber('3.14'),
    tonumber('invalid'),
    tostring(456),
    tostring(true),
    tostring(nil)
}

results[92] = type_conversion_tests

local boundary_tests = {
    math.floor(2147483647),
    math.floor(-2147483648),
    1.7976931348623157e+308,
    2.2250738585072014e-308
}

results[93] = boundary_tests

local empty_containers = {
    {},
    '',
    0,
    false,
    nil
}

results[94] = empty_containers

local algorithm_test = {}
local data = {5, 2, 8, 1, 9, 3}

for i = 1, #data do
    for j = i + 1, #data do
        if data[i] > data[j] then
            data[i], data[j] = data[j], data[i]
        end
    end
end

results[95] = data

local performance_test = 0

for i = 1, 1000 do
    performance_test = performance_test + math.sin(i) * math.cos(i)
end

results[96] = performance_test

local memory_test = {}

for i = 1, 100 do
    memory_test[i] = {
        index = i,
        square = i * i,
        cube = i * i * i,
        text = 'item_' .. tostring(i)
    }
end

results[97] = #memory_test

local final_validation = {
    total_results = #results,
    first_nil = results[1],
    last_number = #memory_test,
    validation_complete = true
}

results[98] = final_validation
results[99] = jit
return {results, true, 1, 'string', {1, 2, 3}, {key1 = true, key2 = 2, key3 = 'value', key4 = {'A', 'B', 'C'}, key5 = {keyA = true, keyB = false}}}