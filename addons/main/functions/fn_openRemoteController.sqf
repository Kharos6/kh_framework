params [["_units", (units (side (group player))) select {!(isPlayer _x);}, [[]]]];

if (_units isEqualTo []) exitWith {
    hint "No units available.";
};

private _display = createDialog ["KH_ResourceRemoteController", true];
ctrlSetText [103, getText ((configOf (_units select 0)) >> "editorPreview")];
private _map = _display displayCtrl 104;
_map ctrlMapAnimAdd [1, 0.25, _units select 0];
ctrlMapAnimCommit _map;

{
    lbAdd [101, [name _x, " - ", getText ((configOf _x) >> "displayName")] joinString ""];
} forEach _units;

lbSetCurSel [101, 0];

[
    ["CONTROL", _display displayCtrl 102],
    "ButtonClick",
    [_units, _display],
    {
        _args params ["_units", "_display"];
        private _unit = _units param [lbCurSel 101, objNull];

        if (alive _unit) then {
            _unit switchCamera "Internal";
            player remoteControl _unit;
            _display closeDisplay 2;
        }
        else {
            hint "This unit is dead.";
        };
    }
] call KH_fnc_addEventHandler;

[
    ["CONTROL", _display displayCtrl 101],
    "LBSelChanged",
    [_units, _display],
    {
        _args params ["_units", "_display"];
        private _unit = _units param [lbCurSel 101, objNull];
        ctrlSetText [103, getText ((configOf _unit) >> "editorPreview")];
        private _map = _display displayCtrl 104;
        ctrlMapAnimClear _map;
        _map ctrlMapAnimAdd [1, 0.25, _unit];
        ctrlMapAnimCommit _map;
    }
] call KH_fnc_addEventHandler;

nil;