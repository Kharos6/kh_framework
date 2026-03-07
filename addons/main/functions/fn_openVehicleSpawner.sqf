params [["_types", [], [[]]], ["_position", objNull, [[], objNull]], ["_rotation", [0, 0, 0], [[], objNull]]];

if (_types isEqualTo []) exitWith {
    hint "No vehicles available.";
};

private _display = createDialog ["KH_ResourceVehicleSpawner", true];
ctrlSetText [103, getText (configFile >> "CfgVehicles" >> (_types param [0, ""]) >> "editorPreview")];

{
    lbAdd [101, getText (configFile >> "CfgVehicles" >> _x >> "displayName")];
} forEach _types;

lbSetCurSel [101, 0];

if (_position isEqualType objNull) then {
    _position = _position modelToWorld [0, 0, 0];
};

_rotation = if (_rotation isEqualType objNull) then {
    [vectorDir _rotation, vectorUp _rotation];
}
else {
	if (_rotation isEqualTypeAll 0) then {
		eulerToVector _rotation;
	}
    else {
        _rotation;
    };
};

[
    ["CONTROL", _display displayCtrl 102],
    "ButtonClick",
    [_types, _display, _position, _rotation],
    {
        _args params ["_types", "_display", "_position", "_rotation"];

        [
            [_position, _rotation, _types param [lbCurSel 101, ""], clientOwner],
            {
                params ["_position", "_rotation", "_selection", "_caller"];
                private _continue = true;

                {
                    if (_x isEqualType "CAManBase") then {
                        _continue = false;
                        break;
                    }
                    else {
                        if ((crew _x) isEqualTo []) then {
                            deleteVehicle _x;
                        }
                        else {
                            _continue = false;
                            break;
                        };
                    };
                } forEach (_position nearEntities [["Air", "LandVehicle", "Ship", "StaticWeapon", "CAManBase"], 20]);

                if _continue then {
                    [
                        [_position, _rotation, _selection],
                        {
                            params ["_position", "_rotation", "_selection"];
                            private _vehicle = createVehicle [_selection, _position, [], 0, "CAN_COLLIDE"];
                            _vehicle setVectorDirAndUp _rotation;
                        },
                        true,
                        "0.5",
                        false
                    ] call KH_fnc_execute;
                }
                else {
                    [
                        [],
                        {
                            hint "SPAWN POSITION OCCUPIED";
                        },
                        _caller,
                        true,
                        false
                    ] call KH_fnc_execute;
                };
            },
            "SERVER",
            true,
            false
        ] call KH_fnc_execute;

        _display closeDisplay 2;
    }
] call KH_fnc_addEventHandler;

[
    ["CONTROL", _display displayCtrl 101],
    "LBSelChanged",
    [_types],
    {
        _args params ["_types"];
        ctrlSetText [103, getText (configFile >> "CfgVehicles" >> (_types param [lbCurSel 101, ""]) >> "editorPreview")];
    }
] call KH_fnc_addEventHandler;

nil;