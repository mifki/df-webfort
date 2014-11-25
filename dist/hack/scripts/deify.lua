-- Renames a random dwarf or deity.
local json = require 'dkjson'
local utils = require 'utils'

function get_hfig(hf_id)
	return utils.binsearch(df.global.world.history.figures, hf_id, 'id')
end

function unit_to_hfig(unit)
	if unit and unit.hist_figure_id then
		return get_hfig(unit.hist_figure_id)
	end
	return nil
end

function get_hfig_name(hfig, in_english)
	return dfhack.TranslateName(hfig.name, in_english or false)
end

function get_citizens()
	local dorfs = {}
	for _, u in ipairs(df.global.world.units.all) do
		if dfhack.units.isCitizen(u) then
			table.insert(dorfs, u)
		end
	end
	return dorfs
end

function get_current_gods()
	local gods = {}
	local dorfs = get_citizens()

	for _, dorf in ipairs(dorfs) do
		local hdorf = unit_to_hfig(dorf)
		for _, link in ipairs(hdorf.histfig_links) do
			if link:getType() == df.histfig_hf_link_type.DEITY then
				gods[link.target_hf] = true
			end
		end
	end

	return gods
end

function make_list(set)
	local list = {}
	for k in pairs(set) do
		table.insert(list, k)
	end
	return list
end

function make_set(list)
	local set = {}
	for _, v in ipairs(list) do
		set[v] = true
	end
	return set
end

function choice(list)
	return list[math.random(#list)]
end

function serialize(state)
	dfhack.persistent.save {
		key = 'deifyState',
		value = json.encode(state)
	}
end

local default = {
	value = [[ { "used_gods": {} } ]]
}
function deserialize()
	return json.decode((dfhack.persistent.get('deifyState') or default).value)
end

function get_new_gods(state, gods)
	local new_gods = {}
	local old_gods = make_set(state.used_gods)
	for god in pairs(gods) do
		if not old_gods[god] then
			new_gods[god] = true
		end
	end

	return make_list(new_gods)
end

function find_named_god(state, name)
	for _, god_id in ipairs(state.used_gods) do
		if get_hfig(god_id).name.first_name == name then
			return god_id
		end
	end

	return nil
end

function name_god(state, god_id, name)
	if find_named_god(state, name) then
		return false
	end
	local godfig = get_hfig(god_id)
	godfig.name.first_name = name
	table.insert(state.used_gods, god_id)
	return true
end

function main(name)
	if not name then
		print("specify a name.")
		return
	end
	local state = deserialize()
	local new_gods = get_new_gods(state, get_current_gods())
	local new_god = choice(new_gods)
	if new_god then
		name_god(state, new_god, name)
	end
	serialize(state)
end

main(...)
