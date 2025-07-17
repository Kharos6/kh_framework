params ["_state", "_machine", "_name"];

if _state then {
	{
		_x addEventHandler [
			"AnimStateChanged", 
			{
				_x setVariable ["KH_var_dataCollectionAnimation", [_this select 1]];
			}
		];

		_x addEventHandler [
			"FiredMan", 
			{
				_x setVariable ["KH_var_dataCollectionFire", [_this select [1]]];
			}
		];

		_x setVariable ["KH_var_dataCollectionId", call KH_fnc_generateUid];
	} forEach KH_var_allEntities;

	KH_var_dataCollectionHandler = [
		{
			if KH_var_dataCollection then {
				{
					private _data = createHashMap;
					_data set ["Transformation", [getPosATL _x, vectorDir _x, vectorUp _x, getObjectScale _x]];
					_data set ["Allegiance", [side _x, group _x]];
					_data set ["Events", _events];
					private _id = _x getVariable "KH_var_dataCollectionId";
					private _currentData = KH_var_collectedData get _id;
					_currentData pushBack [[diag_frameNo, CBA_missionTime], _data];
					KH_var_collectedData set [_id, _currentData];
				} forEach KH_var_allEntities;
			};
		}, 
		0.033, 
		[]
	] call CBA_fnc_addPerFrameHandler;

	KH_var_dataSavingHandler = [
		{
			if KH_var_dataCollection then {
				[format ["%1_collected_data", missionName], "data", KH_var_collectedData, false] call KH_fnc_writeKhData;
			};
		}, 
		60, 
		[]
	] call CBA_fnc_addPerFrameHandler;
};