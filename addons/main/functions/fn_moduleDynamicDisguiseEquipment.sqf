isNil {
	params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

	if _activated then {
		private _side = _logic getVariable ["KH_ModuleDynamicDisguiseEquipmentSide", "BLUFOR"];
		private _uniforms = [];
		private _vests = [];
		private _headgear = [];

		if (isNil "KH_var_disguiseSideUniforms") then {
			KH_var_disguiseSideUniforms = [[], [], []];
		};

		if (isNil "KH_var_disguiseSideVests") then {
			KH_var_disguiseSideVests = [[], [], []];
		};

		if (isNil "KH_var_disguiseSideHeadgear") then {
			KH_var_disguiseSideHeadgear = [[], [], []];
		};

		if (_logic getVariable ["KH_ModuleDynamicDisguiseEquipmentUniforms", false]) then {
			{
				_uniforms pushBackUnique (uniform _x);
			} forEach _units;
		};

		if (_logic getVariable ["KH_ModuleDynamicDisguiseEquipmentVests", false]) then {
			{
				_vests pushBackUnique (vest _x);
			} forEach _units;
		};

		if (_logic getVariable ["KH_ModuleDynamicDisguiseEquipmentHeadgear", false]) then {
			{
				_headgear pushBackUnique (headgear _x);
			} forEach _units;
		};

		switch _side do {
			case "BLUFOR": {
				(KH_var_disguiseSideUniforms select 0) insert [-1, _uniforms select 0, true];
				(KH_var_disguiseSideVests select 0) insert [-1, _vests select 0, true];
				(KH_var_disguiseSideHeadgear select 0) insert [-1, _headgear select 0, true];
			};

			case "OPFOR": {
				(KH_var_disguiseSideUniforms select 1) insert [-1, _uniforms select 1, true];
				(KH_var_disguiseSideVests select 1) insert [-1, _vests select 1, true];
				(KH_var_disguiseSideHeadgear select 1) insert [-1, _headgear select 1, true];
			};

			case "GREENFOR": {
				(KH_var_disguiseSideUniforms select 2) insert [-1, _uniforms select 2, true];
				(KH_var_disguiseSideVests select 2) insert [-1, _vests select 2, true];
				(KH_var_disguiseSideHeadgear select 2) insert [-1, _headgear select 2, true];
			};
		};
	};
};

nil;