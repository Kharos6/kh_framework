params [["_id", [], [[]]]];

if (_id isEqualTypeAll []) exitWith {
    {
        [_x] call KH_fnc_removeHandler;
    } forEach _id;
};

if ((_id select 1) isEqualType []) exitWith {
	_id params [["_type", [], [[]]], ["_handlerId", [], [[]]], ["_persistentExecutionId", "", [""]], ["_eventOwner", nil, [0]]];
	missionNamespace setVariable [_persistentExecutionId, false, true];
	[_handlerId] call KH_fnc_removeHandler;

	if (isNil "_eventOwner") then {
		["KH_eve_eventHandlerRemoved", [_id]] call CBA_fnc_globalEvent;
	}
	else {
		["KH_eve_eventHandlerRemoved", [_id], _eventOwner] call CBA_fnc_ownerEvent;
	};
}
else {
    if ((_id select 0) isEqualType []) then {
        _id params [["_type", [], [[]]], ["_event", "", [true, 0, ""]], ["_handlerId", 0, [0, "", []]], ["_eventOwner", 2, [0]]];

        [
            [_type, _event, _handlerId],
            {
                params ["_type", "_event", "_handlerId"];
                _type params [["_eventType", "", [""]], ["_target", objNull, [true, objNull, grpNull, missionNamespace, locationNull, controlNull, displayNull]]];

                switch _eventType do {
                    case "ENTITY": {
                        _target removeEventHandler [_event, _handlerId];
                    };

                    case "MULTIPLAYER": {
                        _target removeMPEventHandler [_event, _handlerId];
                    };

                    case "CONTROL": {
                        _target ctrlRemoveEventHandler [_event, _handlerId];
                    };

                    case "DISPLAY": {
                        _target displayRemoveEventHandler [_event, _handlerId];
                    };

                    case "PUBLIC_VARIABLE";
                    case "CLASS": {
                        missionNamespace setVariable [_handlerId, false];
                    };

                    case "MISSION": {
                        removeMissionEventHandler [_event, _handlerId];
                    };

                    case "USER_ACTION": {
                        removeUserActionEventHandler [_event, _handlerId];
                    };

                    case "MUSIC": {
                        removeMusicEventHandler [_event, _handlerId];
                    };

                    case "BIS_SCRIPTED": {
                        [_target, _event, _handlerId] call BIS_fnc_removeScriptedEventHandler;
                    };

                    case "TEMPORAL": {
                        ["KH_eve_temporalExecutionStackHandler", [_handlerId, true, false]] call CBA_fnc_localEvent;
                    };

                    case "DRAW_UI": {
                        ["KH_eve_drawUiExecutionStackHandler", [_handlerId, false]] call CBA_fnc_localEvent;
                    };

                    case "IN_GAME_UI": {
                        KH_var_inGameUiEventHandlerStackDeletions pushBackUnique (_handlerId select 0);
                    };

                    case "PLAYER": {
                        [_event, _handlerId] call CBA_fnc_removePlayerEventHandler;
                    };

                    case "CBA": {
                        [_event, _handlerId] call CBA_fnc_removeEventHandler;
                    };
                };

                ["KH_eve_eventHandlerRemoved", [_id]] call CBA_fnc_localEvent;
            },
            _eventOwner,
            true,
            false
        ] call KH_fnc_execute;
    }
    else {
        _id params [
            ["_namespace", objNull, [missionNamespace, objNull, grpNull, teamMemberNull, locationNull, taskNull, controlNull, displayNull]], 
            ["_variable", "", [""]], 
            ["_target", false, [true, 0, []]],
            ["_valueOverride", false, [true]]
        ];

        _namespace setVariable [_variable, _valueOverride, _target];
    };
};