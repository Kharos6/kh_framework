isNil {
	params [["_logic", objNull], ["_units", []], ["_activated", true]];

	if _activated then {
		private _side = (_logic getVariable ["KH_ModuleDynamicDisguiseEquipmentSide", sideUnknown]);
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

		switch true do {
			case (_side == 0): {
				private _uniformsArray = (KH_var_disguiseSideUniforms select 0);
				_uniformsArray insert [-1, _uniforms, true];
				KH_var_disguiseSideUniforms set [0, _uniformsArray];
				private _vestsArray = (KH_var_disguiseSideVests select 1);
				_vestsArray insert [-1, _vests, true];
				KH_var_disguiseSideVests set [1, _vestsArray];
				private _headgearArray = (KH_var_disguiseSideHeadgear select 0);
				_headgearArray insert [-1, _headgear, true];
				KH_var_disguiseSideHeadgear set [0, _headgearArray];
			};

			case (_side == 1): {
				private _uniformsArray = (KH_var_disguiseSideUniforms select 1);
				_uniformsArray insert [-1, _uniforms, true];
				KH_var_disguiseSideUniforms set [1, _uniformsArray];
				private _vestsArray = (KH_var_disguiseSideVests select 1);
				_vestsArray insert [-1, _vests, true];
				KH_var_disguiseSideVests set [1, _vestsArray];
				private _headgearArray = (KH_var_disguiseSideHeadgear select 1);
				_headgearArray insert [-1, _headgear, true];
				KH_var_disguiseSideHeadgear set [1, _headgearArray];
			};

			case (_side == 2): {
				private _uniformsArray = (KH_var_disguiseSideUniforms select 2);
				_uniformsArray insert [-1, _uniforms, true];
				KH_var_disguiseSideUniforms set [2, _uniformsArray];
				private _vestsArray = (KH_var_disguiseSideVests select 1);
				_vestsArray insert [-1, _vests, true];
				KH_var_disguiseSideVests set [1, _vestsArray];
				private _headgearArray = (KH_var_disguiseSideHeadgear select 2);
				_headgearArray insert [-1, _headgear, true];
				KH_var_disguiseSideHeadgear set [2, _headgearArray];
			};
		};
	};
};

true;