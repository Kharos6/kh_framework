params ["_identifier", ["_players", KH_var_allPlayerUnits]];
private _loadoutEntries = [];
private _previousEntriesHash = createHashMapFromArray (missionProfileNamespace getVariable [format ["KH_var_savedPlayerLoadouts_%1", _identifier], []]);

{
	private _playerUid = getPlayerUID _x;
	_loadoutEntries pushBack [_playerUid, getUnitLoadout _x];
	
	if (_playerUid in _previousEntriesHash) then {
		_previousEntriesHash deleteAt _playerUid;
	};
} forEach _players;

private _previousEntriesArray = _previousEntriesHash toArray false;
_loadoutEntries append _previousEntriesArray;

[
	[_identifier, _loadoutEntries], 
	{
		params ["_identifier", "_loadoutEntries"];
		[format ["KH_var_savedPlayerLoadouts_%1", _identifier], _loadoutEntries] call KH_fnc_editMissionProfileNamespace;
	},
	"GLOBAL",
	"THIS_FRAME"
] call KH_fnc_execute;

_loadoutEntries;