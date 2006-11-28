----------------------------------------
-- Copyright (c) Athena Dev Teams - Licensed under GNU GPL
-- For more information, see LICENCE in the main folder
--
-- Athena protocol dissector for Wireshark
-- by FlavioJS
--
-- registered at port 6900 for login
-- registers char&map server ports automatically
-- @see athena_field.char_server.port.callback
-- @see athena_field.map_server.port.callback
--
-- v0.2 - major structure rewrite to try and simplify packet definition
-- v0.1 - initial version to try and find out how to structure the code (with 70 packets)
--
-- TODO add support for split packets (mes and the like)
-- TODO add support for multiple packets in the same frame
-- TODO add more sanity checks
-- TODO add support for multiple packet versions
-- TODO move the logic to a separate lua library file (only packet and field definitions here)
-- TODO standardize  field naming
-- TODO simplify field creation
-- TODO allow reloading the protocol file without restarting wireshark
----------------------------------------
local ports = {  -- ATHENA server ports (only login is required)
	6900
}



----------------------------------------
-- Athena field types
local Field = {}
function Field.UNKNOWN(len)
	return {
		type = "unknown",
		len = len,
		buf = function(self,tvb,off)
			return tvb(off,self.len)
		end,
		value = function(self,tvb,off)
			local buf = self:buf(tvb,off)
			return tostring(buf), buf
		end,
		add_to = function(self,tree,tvb,off)
			local value, buf = self:value(tvb,off)
			if self.len == 1 then
				return tree:add(buf, "Unknown (1 byte): "..value)
			end
			tree = tree:add(buf, "Unknown ("..self.len.." bytes): "..value)
			if self.callback then self:callback(value,buf) end
			return tree
		end,
	}
end
function Field.number(type,len,proto)
	return {
		proto = proto,
		type = type,
		len = len,
		buf = function(self,tvb,off)
			return tvb(off,self.len)
		end,
		value = function(self,tvb,off)
			local buf = self:buf(tvb,off)
			return buf:le_uint(), buf
		end,
		add_to = function(self,tree,tvb,off)
			local value,buf = self:value(tvb,off)
			tree = tree:add(self.proto, buf, value)
			if self.callback then self:callback(value,buf) end
			return tree
		end,
	}
end
function Field.UINT8(abbr,name)
	return Field.number("uint8", 1, ProtoField.uint8(abbr,name))
end
function Field.UINT16(abbr,name)
	return Field.number("uint16", 2, ProtoField.uint16(abbr,name))
end
function Field.UINT32(abbr,name)
	return Field.number("uint32", 4, ProtoField.uint32(abbr,name))
end
function Field.STRING(len,abbr,name,desc)
	return {
		proto = ProtoField.string(abbr,name,desc),
		type = "string",
		len = len,
		buf = function(self,tvb,off)
			return tvb(off,self.len)
		end,
		value = function(self,tvb,off)
			local buf = self:buf(tvb,off)
			return buf:string(), buf
		end,
		add_to = function(self,tree,tvb,off)
			local value,buf = self:value(tvb,off)
			tree = tree:add(self.proto, buf, value)
			if self.callback then self:callback(value,buf) end
			return tree
		end,
	}
end
function Field.ENUM(len,values,abbr,name)
	return {
		proto = ProtoField.string(abbr,name),
		type = "enum",
		len = len,
		buf = function(self,tvb,off)
			return tvb(off,self.len)
		end,
		value = function(self,tvb,off)
			local buf = self:buf(tvb,off)
			local value = values[buf:le_uint()] or buf:le_uint()
			return value, buf
		end,
		add_to = function(self,tree,tvb,off)
			local value,buf = self:value(tvb,off)
			tree = tree:add(self.proto, buf, value)
			if self.callback then self:callback(value,buf) end
			return tree
		end,
	}
end
function Field.BOOL(len,abbr,name)
	return Field.ENUM(len, {[0]='false','true'}, abbr, name)
end
function Field.IPV4(abbr,name)
	return {
		-- proto = ProtoField.ipv4(abbr,name), not yet supported
		proto = ProtoField.string(abbr,name),
		type = "ipv4",
		len = 4,
		buf = function(self,tvb,off)
			return tvb(off,self.len)
		end,
		value = function(self,tvb,off)
			local buf = self:buf(tvb,off)
			local ip = {
				tvb(off+0,1):uint(),
				tvb(off+1,1):uint(),
				tvb(off+2,1):uint(),
				tvb(off+3,1):uint(),
			}
			return string.format("%d.%d.%d.%d",ip[1],ip[2],ip[3],ip[4]), buf
		end,
		add_to = function(self,tree,tvb,off)
			local value,buf = self:value(tvb,off)
			tree = tree:add(self.proto, buf, value)
			if self.callback then self:callback(value,buf) end
			return tree
		end,
	}
end
function Field.COMPOSITE(fields)
	local proto = {}
	local len = 0
	for _,field in ipairs(fields) do
		table.insert(proto,field.proto)
		len = len + field.len
	end
	return {
		type = "composite",
		len = len,
		buf = function(self,tvb,off)
			return tvb(off,self.len)
		end,
		value = function(self,tvb,off)
			return fields.title, self:buf(tvb,off)
		end,
		add_to = function(self,tree,tvb,off)
			local value,buf = self:value(tvb,off)
			tree = tree:add(buf, value)
			for _,field in ipairs(fields) do
				field:add_to(tree, tvb, off)
				off = off + field.len
			end
			if self.callback then self:callback(value,buf) end
			return tree
		end
	}
end

----------------------------------------
-- Create the protocol
local athena_proto = Proto('athena','ATHENA','Athena Protocol')

-- declare the fields
local athena_field = {
	packet_id = Field.UINT16('athena.packet', 'Packet ID'),
	packet_len = Field.UINT16('athena.len', 'Packet Length'),
	version = Field.UINT32('athena.version', 'Packet Version'),
	username = Field.STRING(24, 'athena.username', 'Username'),
	password = Field.STRING(24, 'athena.password', 'Password'),
	login_id1 = Field.UINT32('athena.login_id1', 'Login ID1'),
	login_id2 = Field.UINT32('athena.login_id2', 'Login ID2'),
	account_id = Field.UINT32('athena.account_id', 'Account ID'),
	sex = Field.ENUM(1,{[0]='Female','Male'},'athena.sex', 'Sex'),
	char_server = {
		ip = Field.IPV4('athena.char_server.ip', 'IP'),
		port = Field.UINT16('athena.char_server.port', 'Port'),
		name = Field.STRING(20,'athena.char_server.name', 'Name'),
		users = Field.UINT16('athena.char_server.users', 'Users'),
		maintenance = Field.BOOL(2,'athena.char_server.maintenance', 'Is in maintenance'),
		new = Field.BOOL(2,'athena.char_server.new', 'Is new'),
	},
	char_id = Field.UINT32('athena.char_id', 'Character ID'),
	base_exp = Field.UINT32('athena.base_exp', 'Base Experience'),
	zeny = Field.UINT32('athena.zeny', 'Zeny'),
	job_exp = Field.UINT32('athena.job_exp', 'Job Experience'),
	job_level = Field.UINT32('athena.job_level', 'Job Level'),
	option = Field.UINT32('athena.option', 'Option'),
	karma = Field.UINT32('athena.karma', 'Karma'),
	manner = Field.UINT32('athena.manner', 'Manner'),
	status_point = Field.UINT16('athena.status_point', 'Status points'),
	hp = Field.UINT16('athena.hp', 'HP'),
	max_hp = Field.UINT16('athena.max_hp', 'Maximum HP'),
	sp = Field.UINT16('athena.sp', 'SP'),
	max_sp = Field.UINT16('athena.max_sp', 'Maximum SP'),
	walk_speed = Field.UINT16('athena.walk_speed', 'Walk speed'),
	class = Field.UINT16('athena.class', 'Class'),
	hair_style = Field.UINT16('athena.hair_style', 'Hair style'),
	weapon = Field.UINT16('athena.weapon', 'Weapon'),
	base_level = Field.UINT16('athena.base_level', 'Base level'),
	skill_point = Field.UINT16('athena.skill_point', 'Skill points'),
	head_bottom = Field.UINT16('athena.head_bottom', 'Head bottom'),
	shield = Field.UINT16('athena.shield', 'Shield'),
	head_top = Field.UINT16('athena.head_top', 'Head top'),
	head_mid = Field.UINT16('athena.head_mid', 'Head mid'),
	hair_color = Field.UINT16('athena.hair_color', 'Hair color'),
	clothes_color = Field.UINT16('athena.clothes_color', 'Clothes color'),
	name = Field.STRING(24,'athena.name', 'Name'),
	str = Field.UINT8('athena.str', 'STR'),
	agi = Field.UINT8('athena.agi', 'AGI'),
	vit = Field.UINT8('athena.vit', 'VIT'),
	int = Field.UINT8('athena.int', 'INT'),
	dex = Field.UINT8('athena.dex', 'DEX'),
	luk = Field.UINT8('athena.luk', 'LUK'),
	char_num = Field.UINT8('athena.char_num', 'Character number'),
	client_tick = Field.UINT32('athena.client_tick', 'Client tick'),
	selected_char = Field.UINT8('athena.selected_char', 'Selected character'),
	map_name = Field.STRING(16,'athena.map_name', 'Map name'),
	map_server = {
		ip = Field.IPV4('athena.map_server.ip', 'IP'),
		port = Field.UINT16('athena.map_server.port', 'Port'),
	},
	type = Field.ENUM(2,{
		[0]='SP_SPEED',
		[3]='SP_KARMA',
		[4]='SP_MANNER',
		[5]='SP_HP',
		[6]='SP_MAXHP',
		[7]='SP_SP',
		[8]='SP_MAXSP',
		[9]='SP_STATUSPOINT',
		[11]='SP_BASELEVEL',
		[12]='SP_SKILLPOINT',
		[24]='SP_WEIGHT',
		[25]='SP_MAXWEIGHT',
		[41]='SP_ATK1',
		[42]='SP_ATK2',
		[43]='SP_MATK1',
		[44]='SP_MATK2',
		[45]='SP_DEF1',
		[46]='SP_DEF2',
		[47]='SP_MDEF1',
		[48]='SP_MDEF2',
		[49]='SP_HIT',
		[50]='SP_FLEE1',
		[51]='SP_FLEE2',
		[52]='SP_CRITICAL',
		[53]='SP_ASPD',
		[55]='SP_JOBLEVEL',
	},'athena.type', 'Type'),
	value = Field.UINT32('athena.value', 'Value'),
	server_tick = Field.UINT32('athena.server_tick', 'Server tick'),    
}
do
	local mt = {
		__index = function(t,k)
			error('Field "'..k..'" does not exist',2)
		end
	}
	setmetatable(athena_field,mt)
end
-- special type, for dynamic packets
athena_field.packet_len.type = 'length'
-- set the protocol fields
do
	local proto_fields = {}
	local function add_field(field)
		if field.type then
			if field.proto then
				table.insert(proto_fields, field.proto)
			end
		else -- table of fields
			for _,subfield in pairs(field) do
				add_field(subfield)
			end
		end
	end
	for _,field in pairs(athena_field) do
		add_field(field)
	end
	athena_proto.fields = proto_fields
end

local parsers = {} -- packet parsing functions
local dynamic_len = {} -- packet size of dynamic packets
-- Packet creation and parsing function
local function Packet(p)
	-- sanity checks
	if not p.id then error("Missing packet ID", 2) end
	if not p.len then error("Missing packet length", 2)	end
	if p.len == 0 then error("Invalid packet length 0", 2) end
	if p.len <= 0 and p[1].type ~= 'length' then error("Dynamic packets need to start with the packet length field", 2) end
	-- normalize packet id (expects a 4 digit hex string)
	if type(p.id) == 'number' then
		p.id = string.format("%04X",p.id)
	elseif type(p.id) == 'string' then
		p.id = string.upper(tostring(p.id))
	else error("Unsupported type for packet id '"..type(p.id).."'", 2) end
	-- create packet parser
	parsers[p.id] = function(tvb,pinfo,tree)
		-- mark packet protocol
		pinfo.cols.protocol = "ATHENA"
		if not pinfo.visited and p.len > 0 then
			local athena_tree = tree:add(athena_proto)
			return -- no further processing required
		end
		-- packet length
		local len = dynamic_len[pinfo.number] or p.len
		if len < 0 then -- get length of the dynamic packet
			len = p[1]:value(tvb,2)
			dynamic_len[pinfo.number] = len
		end
		local athena_tree = tree:add(athena_proto,tvb(0,len),"Athena Packet ("..p.id..") - "..p.desc)
		athena_field.packet_id:add_to(athena_tree,tvb,0)
		local off = 2
		for _,field in ipairs(p) do
			field:add_to(athena_tree,tvb,off)
			off = off + field.len
		end
		if p.callback then p:callback(tvb,pinfo,tree) end
	end
end

-- Athena packet dissector
local custom_parsers = {} -- custom parsers for specific frames
local next_parser -- next custom parser, before going to custom_parsers
function athena_proto.dissector(tvb,pinfo,tree)
	if tvb:len() == 0 then return end
	if next_parser then -- inject the custom parser
		custom_parsers[pinfo.number] = next_parser
		next_parser = nil
	end
	if custom_parsers[pinfo.number] then
		-- custom packet parser
		local parser = custom_parsers[pinfo.number]
		return parser(tvb,pinfo,tree)
	end
	local packet_id = athena_field.packet_id
	if tvb:len() < packet_id.len then return end -- too small
	local id = string.format('%04X', packet_id:value(tvb,0))
	local parse = parsers[id]
	if parse then
		parse(tvb,pinfo,tree)
	else
		pinfo.cols.protocol = "ATHENA"
		tree:add(athena_proto,tvb(), "Athena Packet ???")
	end
end

-- Register ports
local tcp_table = DissectorTable.get("tcp.port")
do -- dynamic port registry
	-- char server
	local char_ports = {}
	athena_field.char_server.port.callback = function(self,value,buf)
		if not char_ports[value] then
			print("Registered: Char server at port",value)
			tcp_table:add(value, athena_proto)
			char_ports[value] = true
		end
	end
	-- map server
	local map_ports = {}
	athena_field.map_server.port.callback = function(self,value,buf)
		if not map_ports[value] then
			print("Registered: Map server at port",value)
			tcp_table:add(value, athena_proto)
			map_ports[value] = true
		end
	end
end
-- ports at the begining of the file
if ports == nil then ports = {} end
for _,port in ipairs(ports) do
	tcp_table:add(port, athena_proto)
end










--------------------------------------------------
-- Packet Definitions
-- this is where the fun begins, oh yeah d(^.^)
--------------------------------------------------

--
-- Login server
--

Packet{
	id = "0064",
	desc = "Request login without encryption",
	len = 55,
	athena_field.version,
	athena_field.username,
	athena_field.password,
	Field.UNKNOWN(1),
}

Packet{
	id = "0069",
	desc = "Login accepted, char server information",
	len = -1,
	athena_field.packet_len,
	athena_field.login_id1,
	athena_field.account_id,
	athena_field.login_id2,
	Field.UNKNOWN(4),
	Field.UNKNOWN(24),
	Field.UNKNOWN(2),
	athena_field.sex,
	Field.COMPOSITE{ -- TODO multiple char servers
		title="Char server",
		athena_field.char_server.ip,
		athena_field.char_server.port,
		athena_field.char_server.name,
		athena_field.char_server.users,
		athena_field.char_server.maintenance,
		athena_field.char_server.new,
	}
}

--
-- Char server
--

local callbacks_0065 = {}
Packet{
	id = "0065",
	desc = "Request char server connection",
	len = 17,
	athena_field.account_id,
	athena_field.login_id1,
	athena_field.login_id2,
	Field.UNKNOWN(2),
	athena_field.sex,
	callback = function(self,tvb,pinfo,tree)
		-- set a custom parser to the next packet
		local number = pinfo.number + 1
		if callbacks_0065[number] then return end -- already done
		callbacks_0065[number] = true
		-- the custom parser, it's injected in custom_parsers after the first packet with len > 0
		next_parser = function(tvb,pinfo,tree)
			local account_id = athena_field.account_id
			if tvb:len() ~= account_id.len then -- wrong packet?
				print("["..pinfo.number.."] Expected an account_id after packet 0065, received "..tvb:len().." bytes instead: ", tvb)
				custom_parsers[pinfo.number] = nil
				return athena_proto.dissector(tvb,pinfo,tree)
			end
			pinfo.cols.protocol = "ATHENA"
			tree = tree:add(athena_proto,tvb(0,len),"Athena Packet - Response of packet 0065")
			account_id:add_to(tree,tvb,0)
		end
	end,
}

Packet{
	id = "006B",
	desc = "Receive character data",
	len = -1,
	athena_field.packet_len,
	Field.UNKNOWN(20),
	Field.COMPOSITE{ -- TODO multiple characters
		title="Character",
		athena_field.char_id,
		athena_field.base_exp,
		athena_field.zeny,
		athena_field.job_exp,
		athena_field.job_level,
		Field.UNKNOWN(4),
		Field.UNKNOWN(4),
		athena_field.option,
		athena_field.karma,
		athena_field.manner,
		athena_field.status_point,
		athena_field.hp,
		athena_field.max_hp,
		athena_field.sp,
		athena_field.max_sp,
		athena_field.walk_speed,
		athena_field.class,
		athena_field.hair_style,
		athena_field.weapon,
		athena_field.base_level,
		athena_field.skill_point,
		athena_field.head_bottom,
		athena_field.shield,
		athena_field.head_top,
		athena_field.head_mid,
		athena_field.hair_color,
		athena_field.clothes_color,
		athena_field.name,
		athena_field.str,
		athena_field.agi,
		athena_field.vit,
		athena_field.int,
		athena_field.dex,
		athena_field.luk,
		athena_field.char_num,
	},
}

Packet{
	id = "0187",
	desc = "Client alive/synchronization packet",
	len = 6,
	athena_field.client_tick,
}

Packet{
	id = "0066",
	desc = "Select Character",
	len = 3,
	athena_field.selected_char,
}

Packet{
	id = "0071",
	desc = "Character selected, map server information",
	len = 28,
	athena_field.char_id,
	athena_field.map_name,
	athena_field.map_server.ip,
	athena_field.map_server.port,
}

--
-- Map server
--

Packet{
	id = "009B",
	desc = "Connect to map server",
	len = 37,
	Field.UNKNOWN(7),
	athena_field.account_id,
	Field.UNKNOWN(8),
	athena_field.char_id,
	Field.UNKNOWN(3),
	athena_field.login_id1,
	athena_field.client_tick,
	athena_field.sex,
}

Packet{
	id = "021D",
	desc = "???",
	len = 6,
	Field.UNKNOWN(4),
}

Packet{
	id = "007D",
	desc = "Load End ACK",
	len = 2,
}

Packet{
	id = "0089",
	desc = "Tick send",
	len = 13,
	Field.UNKNOWN(7),
	athena_field.client_tick,
}


Packet{
	id = "00B0",
	desc = "Update info",
	len = 13,
	athena_field.type,
	athena_field.value,
}
