params ["_id", "_state"];

[
	[_id, _state],
	{
		params ["_id", "_state"];
		private _type = _id select 0;

		switch _type do {
			case "PRIVATE_HANDLER": {
				private _handle = _id select 1;
				private _owner = _id select 2;
				missionNamespace setVariable [_handle, _state, _owner];
			};

			case "JIP_HANDLER": {
				private _variable = _id select 1;
				missionNamespace setVariable [_variable, _state];
			};

			case "PERSISTENT_HANDLER": {
				private _handle = _id select 1;
				private _object = _id select 2;
				_object setVariable [_handle, _state, true];	
			};

			case "PUBLIC_HANDLER": {
				private _variable = _id select 1;
				missionNamespace setVariable [_variable, _state, true];
			};
		};
	},
	"SERVER",
	true
] call KH_fnc_execute;