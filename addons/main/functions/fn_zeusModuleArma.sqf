isNil {
	params [["_logic", objNull]];
	private _entity = attachedTo _logic;
	deleteVehicle _logic;

	if !(_entity isKindOf "Man") then {
		[_entity] call KH_fnc_arma;
	};
};

true;