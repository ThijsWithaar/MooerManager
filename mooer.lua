--[[
In Wireshark, Help/About/Folders/Lua plugins shows where this script should be copied to.
Press CTRL-SHIFT-L to reload the script while whireshark is running.

Filter the packets in wireshark with one of these lines:
	(usb.device_address == 5) && (usb.data_len > 4)
	usb.src != 1.2.2 && (usb.dst == 1.5.2 || usb.dst==host) && usb.data_len > 0
	usb.data_len > 8
	usb.src != 1.2.1 && usb.src != 1.2.2 && usb.dst != 1.2.1 && usb.dst != 1.2.2 && usb.data_len > 0

With this plugin loaded, filtering on events is possible, e.g:
	mooer.opcode > 0
]]--


function dict_to_string(o)
	local s = " "
	for k,v in pairs(o) do
		s = s .. k .. " " .. v .. ", "
	end
  return s
end


usb_mooer_protocol = Proto("MOOER_GE200",  "Mooer GE200 USB")


-- What the PC sends to the MOOER
local mooer_opcodes = {
	[3] = "RequestIdentity",
	[1] = "RequestPatchList",
	[2] = "SendPatchChangeA",
	[5] = "SendPatchChangeB",
	[18] = "RespondIdentity",
	[0x82] = "Change Menu",
}

-- What the MOOER sends to the PC
local rx_codes = {
	[0xA5] = "PatchSettings",
	[0x10] = "Identify",
	[0xA6] = "PatchChange?",
	[0x93] = "Pedal",
}

local fx_ids = {
	[0x90] = "FX",
	[0x91] = "DS/OD",
	[0x93] = "AMP",
	[0x94] = "CAB",
	[0x95] = "NS GATE",
	[0x96] = "EQ",
	[0x97] = "MOD",
	[0x98] = "DELAY",
	[0x99] = "REVERB",
}


local packet_length = ProtoField.uint8("mooer.packet_length", "Packet Length", base.DEC)
local hdr = ProtoField.uint8("mooer.header", "header",   base.DEC)
local opcode = ProtoField.uint8("mooer.opcode", "OpCode", base.DEC, mooer_opcodes)
local p_index = ProtoField.uint8("mooer.index", "Index", base.DEC)
local rxcode = ProtoField.uint16("mooer.rxcode", "Response Code", base.HEX, rx_codes)

local id_version = ProtoField.string("mooer.id_version", "Identify Version")
local id_name = ProtoField.string("mooer.id_name", "Identify Name")

local patch_length = ProtoField.uint16("mooer.patch_len", "Patch Length", base.DEC)
local patch_idx = ProtoField.uint8("mooer.patch_idx", "Patch Index")
local patch_name = ProtoField.string("mooer.patch_name", "Patch Name")
local patch_check = ProtoField.uint16("mooer.patch_chk", "Patch Checksum", base.HEX)

local expr_a = ProtoField.int16("mooer.expr_a", "Expression Pedal A", base.DEC)
local expr_b = ProtoField.int16("mooer.expr_b", "Expression Pedal B", base.DEC)


usb_mooer_protocol.fields = { packet_length, opcode, p_index, rxcode,
	id_version, id_name,
	patch_length, patch_idx, patch_name, patch_check,
	expr_a, expr_b
}


-- variables to persist across all packets
local pkt_data = {} -- indexed per packet


function usb_mooer_protocol.dissector(buffer, pinfo, tree)
	length = buffer:len()
	if length < 4 then return end

	pinfo.cols.protocol = usb_mooer_protocol.name
	local subtree = tree:add(usb_mooer_protocol, buffer(), "Mooer GE200")

	subtree:add_le(packet_length, buffer(0, 1))

	local hdr_id = buffer(1,2):uint()

	if (tostring(pinfo.src) == "host") and (hdr_id == 0xAA55) then
		-- The host commands
		local opcode_id = buffer(4, 1):le_uint() -- This is the message length
		local opid2 = buffer(5,1):uint()
		local index = buffer(6,1):uint()
		subtree:add_le(opcode, opid2)
		subtree:add_le(p_index, index)
		pinfo.cols["info"] = string.format("cmd=%02X ", opid2)
		if opcode_id == 18 then
		elseif opcode_id == 2 then
			slot_idx = buffer(6,1):uint()
			block_idx = buffer(7,1):uint()
			pinfo.cols["info"]:append(string.format("AMP upload into slot %i.%i", slot_idx, block_idx))
		elseif opid2 == 0xA3 then
			local fx_names = {
				[0] = "FX/COMP",
				[1] = "DS/OD",
				[2] = "AMP",
				[3] = "CAB",
				[4] = "NS",
				[5] = "EQ",
				[6] = "MOD",
				[7] = "DELAY",
				[8] = "REVERB",
			}
			local fx_params = {
				[0] = {"POSITION","Q","RATE","ATTACK","RANGE","SENSE","THRE","PEAK","RATIO","LEVEL"},
				[1] = {"VOLUME", "TONE", "GAIN"},
				[2] = {"GAIN", "BASS", "MID", "TREBLE", "PRESS", "MST" },
				[3] = {"CENTER", "DISTANCE"},
				[4] = {"THRES", "SENS", "ATTACK", "RELEASE"},
				[5] = {"100 Hz", "80 Hz", "250 Hz", "240 Hz",
					"200 Hz", "630 Hz", "750 Hz", "400 Hz",
					"1.6 kHz", "2.2 kHz", "800 Hz", "4kHz",
					"6.6 kHz", "3.2 kHZ"
					},
				[6] = {"RATE", "PITCH", "RISE", "SAMPLE", "MIX",
					"DEPTH", "LEVEL", "FEEDBACK", "TONE", "Q",
					"RANGE", "BIT"},
				[7] = {"LEVEL", "F.BACK", "TIME", "TIME A", "THRE", "TIME B"},
				[8] = {"PRE DELAY", "LEVEL", "DECAY", "TONE"},
			}

			on_off = buffer(6,1):uint()
			exp_idx = buffer(7,1):uint()
			exp_knob = buffer(8,1):uint()
			exp2_idx = buffer(9,1):uint()
			exp2_knob = buffer(10,1):uint()
			pd_min = buffer(11,1):uint()
			pd_max = buffer(12,1):uint()
			pinfo.cols["info"]:append(string.format("%i EXP %s %s, EXP2 %s, ped %i .. %i",
				on_off, fx_names[exp_idx], fx_params[exp_idx][exp_knob+1], fx_names[exp2_idx], pd_min, pd_max))
		elseif opcode_id == 2 then
		subtree:add_le(patch_idx, buffer(6, 1))
		subtree:add_le(patch_check, buffer(7, 2))
		elseif opcode_id == 5 then
			menu_idx = buffer(6,1):uint()
			pinfo.cols["info"]:append(string.format("Menu %i", menu_idx))
		elseif opcode_id == 0x0B then
			effect_id = buffer(5,1):uint()
			on_off = buffer(7,1):uint()
			pinfo.cols["info"]:append(string.format("SetEffect %02x, on %i", effect_id, on_off))
		elseif opcode_id == 0x0D then
			effect_id = buffer(5,1):uint()
			on_off = buffer(7,1):uint()
			pinfo.cols["info"]:append(string.format("SetPatch %02x, on%i", effect_id, on_off))
		elseif opcode_id == 0x11 then
			pinfo.cols["info"]:append("AMP") -- Not quite right
		end
	elseif (tostring(pinfo.src) ~= "host") then
	-- The responses from the GE200
		msg_length = buffer(3,2):uint()
	rx_code_val = buffer(5,1):uint()
	pkt_data[pinfo.number] = buffer -- Store buffer for merging

	subtree:add_le(rxcode, rx_code_val)
	pinfo.cols["info"] = string.format("rxc=%02X ", rx_code_val)

	on_off = buffer(7,1):uint()

	if packet_length == 1 then
	elseif rx_code_val == 0xA5 then
		subtree:add_le(patch_length, buffer(3, 2))
		subtree:add_le(patch_idx, buffer(6, 1))
		subtree:add_le(patch_name, buffer(19, 14))
		patch_name = buffer(19, 14):string()
		pinfo.cols["info"]:append(string.format("Patch %s", patch_name))
	elseif rx_code_val == 0x10 then
		subtree:add_le(id_version, buffer(7, 5))
		subtree:add_le(id_name, buffer(12, 11))
	elseif rx_code_val == 0xA6 then
		subtree:add_le(patch_idx, buffer(6, 1))
	elseif rx_code_val == 0xE2 then
		block_idx = buffer(6,1):uint()
		pinfo.cols["info"]:append(string.format("Upload received %i", block_idx))
	elseif rx_code_val == 0x82 then
		menu_idx = buffer(6,1):uint()
		pinfo.cols["info"]:append(string.format("Menu %i", menu_idx))
	elseif rx_code_val == 0x90 then
		fx_settings = {
			['on'] = on_off,
			['model'] = buffer(9,1):uint(),
			['attack'] = buffer(11,1):uint(),
		}
		pinfo.cols["info"]:append("FX " .. dict_to_string(fx_settings))
	elseif rx_code_val == 0x91 then
		dist_settings = {
			['on'] = on_off,
			['model'] = buffer(9,1):uint(),
			['vol'] = buffer(11,1):uint(),
			['tone'] = buffer(13,1):uint(),
			['gain'] = buffer(15,1):uint(),
		}
		pinfo.cols["info"]:append("DS " .. dict_to_string(dist_settings))
	elseif rx_code_val == 0x93 then
		amp_settings = {
			['on'] = on_off,
			['gain'] = buffer(11,1):uint(),
			['bass'] = buffer(13,1):uint(),
			['mid'] = buffer(15,1):uint(),
			['treble'] = buffer(17,1):uint(),
			['pres'] = buffer(19,1):uint(),
		}
		pinfo.cols["info"]:append("AMP " .. dict_to_string(amp_settings))
	elseif rx_code_val == 0x96 then
		eq_settings = {
			['on'] = on_off,
			['model'] = buffer(9,1):uint(),
			['band1'] = buffer(11,1):uint(),
		}
		pinfo.cols["info"]:append("EQ " .. dict_to_string(eq_settings))
    end
  end
end


-- via vendor/product ID: Works for windows-captures, but not for linux?
local usb_product_dissectors = DissectorTable.get("usb.product")
usb_product_dissectors:add(0x04835703, usb_mooer_protocol)

-- hid++ 2.0
local IF_CLASS_UNKNOWN          = 0xFFFF
--DissectorTable.get("usb.interrupt"):add(IF_CLASS_UNKNOWN, usb_mooer_protocol)
--DissectorTable.get("usb.control"):add(IF_CLASS_UNKNOWN, usb_mooer_protocol)

-- hid++ 1.0
local HID                       = 0x0003
-- DissectorTable.get("usb.interrupt"):add(HID, usb_mooer_protocol)
-- DissectorTable.get("usb.control"):add(HID, usb_mooer_protocol)
