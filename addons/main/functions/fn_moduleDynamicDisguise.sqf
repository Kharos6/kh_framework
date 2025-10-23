isNil {
	params [["_logic", objNull, [objNull]], ["_units", [], [[]]], ["_activated", true, [true]]];

	if _activated then {
        [
            [
                parseSimpleArray (["[", _logic getVariable ["KH_ModuleDynamicDisguiseBluforUniforms", ""], "]"] joinString ""), 
                parseSimpleArray (["[", _logic getVariable ["KH_ModuleDynamicDisguiseOpforUniforms", ""], "]"] joinString ""), 
                parseSimpleArray (["[", _logic getVariable ["KH_ModuleDynamicDisguiseGreenforUniforms", ""], "]"] joinString "")
            ],
            [
                parseSimpleArray (["[", _logic getVariable ["KH_ModuleDynamicDisguiseBluforVests", ""], "]"] joinString ""), 
                parseSimpleArray (["[", _logic getVariable ["KH_ModuleDynamicDisguiseOpforVests", ""], "]"] joinString ""), 
                parseSimpleArray (["[", _logic getVariable ["KH_ModuleDynamicDisguiseGreenforVests", ""], "]"] joinString "")
            ],
            [
                parseSimpleArray (["[", _logic getVariable ["KH_ModuleDynamicDisguiseBluforHeadgear", ""], "]"] joinString ""), 
                parseSimpleArray (["[", _logic getVariable ["KH_ModuleDynamicDisguiseOpforHeadgear", ""], "]"] joinString ""), 
                parseSimpleArray (["[", _logic getVariable ["KH_ModuleDynamicDisguiseGreenforHeadgear", ""], "]"] joinString "")
            ],
            _logic getVariable ["KH_ModuleDynamicDisguiseCaptive", false]
        ] call KH_fnc_dynamicDisguise;
	};
};

nil;