params [["_attributes", [], [[]]], ["_overrideAttributes", [], [[]]]];
private _savedAttributes = _attributes param [1];

if (_overrideAttributes isNotEqualTo []) then {
	private _currentAttributes = ([_object] call KH_fnc_getMissionAttributes) select 3;

	{
		if (_x isEqualType 0) then {
			_savedAttributes set [_x, _currentAttributes select _x];
		}
		else {
			_x params [["_index", 0, [0]], "_value"];
			_savedAttributes set [_index, _value];
		};
	} forEach _overrideAttributes;
};

_savedAttributes params [
    "_environmentEnabled",
    "_timeMultiplier",
    "_date",
    "_fogParams",
    "_wind",
    "_windDir",
    "_gusts",
    "_overcast",
    "_lightnings",
    "_rainParams",
    "_rainbow",
    "_humidity",
    "_waves",
    "_sideRelations",
    "_variables"
];

setTimeMultiplier _timeMultiplier;
setDate _date;
0 setFog _fogParams;
setWind _wind;
0 setWindDir _windDir;
0 setGusts _gusts;
0 setOvercast _overcast;
0 setLightnings _lightnings;
setRain _rainParams;
0 setRainbow _rainbow;
setHumidity _humidity;
0 setWaves _waves;
forceWeatherChange;

[
    [_environmentEnabled],
    {   
        params ["_environmentEnabled"];
        enableEnvironment _environmentEnabled;
    },
    true,
    {time > 1;},
    false
] call KH_fnc_execute;

{
    _x params ["_side1", "_side2", "_relationship"];
    _side1 setFriend [_side2, _relationship];
} forEach _sideRelations;

{
    missionNamespace setVariable _x;
} forEach _variables;

[
	[
        _environmentEnabled,
        _date,
        _gusts,
        _overcast,
        _lightnings,
        _rainParams,
        _rainbow,
        _humidity,
        _waves
	], 
	{
		params [
            "_environmentEnabled",
            "_date",
            "_gusts",
            "_overcast",
            "_lightnings",
            "_rainParams",
            "_rainbow",
            "_humidity",
            "_waves"
		];
        
		[
			[_environmentEnabled],
			{   
                params ["_environmentEnabled"];
                enableEnvironment _environmentEnabled;
			},
			true,
			{time > 1;},
			false
		] call KH_fnc_execute;

        setDate _date;
        0 setGusts _gusts;
        0 setOvercast _overcast;
        0 setLightnings _lightnings;
        setRain _rainParams;
        0 setRainbow _rainbow;
        setHumidity _humidity;
        0 setWaves _waves;
	}, 
	"REMOTE", 
	true, 
	false
] call KH_fnc_execute;

nil;