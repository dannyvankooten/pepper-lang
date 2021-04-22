function sort( f )
    for k = 1, #f-1 do   
        -- find idx of min value
        local idx = k    
        for i = k+1, #f do
            if f[i] < f[idx] then 
                idx = i
            end    
        end

        -- swap
        tmp = f[k]
        f[k] = f[idx]
        f[idx] = tmp 
    end
end

-- read entire file 
local file = io.open("numbers.txt", "r") 
local content = file:read "*a"
file:close()

-- split by comma, convert to number
local t={}
for str in string.gmatch(content, "([^, ]+)") do
    table.insert(t, tonumber(str))
end

sort(t)

print(t[1], " - ", t[5000])