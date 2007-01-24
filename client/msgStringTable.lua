--
-- Copyright (c) 2007 Athena Dev Teams
--
-- Permission is hereby granted, free of charge, to any person obtaining a 
-- copy of this work to deal in this work without restriction (including the 
-- rights to use, modify, distribute, sublicense, and/or sell copies).
--
-- Description: extracts the msgStringTable that is embeded in the client exe
-- Structure:
--     each entry has an <uint32 index> and a <char* msg>
--     index starts at 0 and increases linearly by 1
-- Note: the extracted strings will maintain the encoding they had in the exe 
--       (probably EUC-KR)
--
-- Author: FlavioJS
--



---------------
-- Configure --



local exe = 'Sakexe.exe' -- What file to read
-- Uncomment the files you want to generate
--local txtfile = 'msgStringTable.txt' -- Output file for entries separated by '#'
--local cfile = 'msgStringTable.c' -- Output file for a C structure



------------------
-- Read the exe --



-- read the exe
print('Reading "'..exe..'"...')
local data
do
	local f,err = io.open(exe,'rb')
	if f == nil then
		print('Error:',err)
		return
	end
	data = f:read('*a')
	f:close()
end
print("size = "..#data.." bytes")



---------------------
-- Search the data --



-- dword iterator, returns (offset,dword)
local function dwords(data)
	local off = 1
	return function()
		if #data < off + 3 then
			return nil
		end
		local a,b,c,d = string.byte(data, off, off+3)
		local k = off
		local v = (a + b*0x100 + c*0x10000 + d*0x1000000)
		off = off + 4
		return k,v
	end
end

-- Returns the list of dword of the msgstringtable (the biggest one found)
local function getBiggestTable(data)
	local off = 0
	local i = 0
	local index = true
	local tbl = {}
	local max = {}
	for key,val in dwords(data) do
		if index and val == i then
			-- index found
			if val == 0 then
				off = key
			end
			i = i + 1
			index = false
		elseif not index and val >= 0x400000 then
			-- valid offset
			table.insert(tbl, val)
			index = true
		elseif val == 0 then
			-- invalid index or offset and start of new seguence
			if #tbl > #max then
				max = tbl
			end
			off = key
			i = 1
			tbl = {}
			index = false
		else
			-- invalid index or offset
			if #tbl > #max then
				max = tbl
			end
			i = 0
			tbl = {}
			index = true
		end
	end
	return max
end

print('Processing...')
local tbl = getBiggestTable(data)
print('entries = '..tostring(#tbl))



---------------------
-- Dump the result --



-- Returns the C string in data starting at off
local function bytestring(data,off)
	if off > #data then return "" end
	local len = 0
	while data:byte(off+len) > 0 do
		len = len + 1
	end
	return string.char(data:byte(off,off+len-1))
end

-- Simple string escape, add other cases when needed
local function escape(str)
	str = string.gsub(str, '\\', '\\\\')
	str = string.gsub(str, '"', '\\"')
	str = string.gsub(str, string.char(9), '\\t')
	str = string.gsub(str, string.char(10), '\\n')
	str = string.gsub(str, string.char(13), '\\r')
	return '"'..str..'"'
end

-- Dump the txt file
if txtfile then
	print('Dumping to "'..txtfile..'"...')
	local f,err = io.open(txtfile, 'w')
	if f == nil then
		print('Error:',err)
		return
	end
	for i = 1, #tbl do
		local off = tbl[i] - 0x400000 + 1
		local str = bytestring(data,off)
		f:write(str,'#\n')
	end
	f:close()
end

-- Dump the C file
if cfile then
	print('Dumping to "'..cfile..'"...')
	local f,err = io.open(cfile, 'w')
	if f == nil then
		print('Error:',err)
		return
	end
	f:write('struct {\n','\tunsigned int index;\n','\tchar* msg;\n','} msgStringTable = {\n')
	for i = 1, #tbl do
		local off = tbl[i] - 0x400000 + 1
		local str = bytestring(data,off)
		if i ~= #tbl then
			f:write(string.format('\t{\t%d,\t%s\t},\n',i-1,escape(str)))
		else
			f:write(string.format('\t{\t%d,\t%s\t}\n',i-1,escape(str)))
		end
	end
	f:write('}\n')
	f:close()
end



----------
-- Done --



print('Done')
