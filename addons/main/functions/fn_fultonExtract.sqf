params [
	["_object", objNull, [objNull]], 
	["_vehicles", [], [[]]], 
	["_height", 100, [0]], 
	["_distance", 0, [0]], 
	["_maximumParticipants", 10, [0]], 
	["_duration", 15, [0]], 
	["_objectName", "", [""]]
];

_height = _height min 100;
private _fultonEvent = generateUid;
private _fultonId = generateUid;
missionNamespace setVariable [fultonId, false];

[
	[
		_object,
		["Prime Fulton ", _objectName] joinString "",
		"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
		"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
		["(!(missionNamespace getVariable ['", _fultonId, "', false]) && ((_this distance _target) < 4))"] joinString "",
		["(!(missionNamespace getVariable ['", _fultonId, "', false]) && ((_caller distance _target) < 4))"] joinString "",
		{},
		{},
		{
			(_this select 3) params ["_fultonEvent"];
			[_fultonEvent, []] call CBA_fnc_serverEvent;
		},
		{},
		[_fultonEvent],
		6,
		0,
		false,
		false,
		true
	],
	"BIS_fnc_holdActionAdd",
	"PLAYERS",
	true,
	["JIP", _object, false, ""]
] call KH_fnc_execute;

[
	"CBA",
	_fultonEvent,
	[_object, _vehicles, _height, _distance, _maximumParticipants, _duration, _objectName, _fultonId],
	{
		_args params ["_object", "_vehicles", "_height", "_distance", "_maximumParticipants", "_duration", "_objectName", "_fultonId"];
		missionNamespace setVariable [_fultonId, true, true];
		private _mainFulton = createVehicle ["B_Parachute_02_F", _object, [], 0, "CAN_COLLIDE"];
		_mainFulton allowDamage false;
		private _fultonActive = generateUid;
		private _currentFultonParticipants = generateUid;
		private _fultonBox = generateUid;
		private _mainFultonVelocity = generateUid;
		private _fultonRemoteActions = generateUid;
		missionNamespace setVariable [_fultonActive, false, true];
		missionNamespace setVariable [_currentFultonParticipants, [], true];
		missionNamespace setVariable [_fultonBox, _object, true];
		missionNamespace setVariable [_mainFultonVelocity, 0.1];
		missionNamespace setVariable [_fultonRemoteActions, false];
		private _fultonAnchor = createVehicle ["Land_Can_V2_F", _object, [], 0, "CAN_COLLIDE"];
		_fultonAnchor allowDamage false;
		_fultonAnchor hideObjectGlobal true;												
		_fultonAnchor attachTo [_object, [0, 0, 0]];
		private _fultonRope = ropeCreate [_mainFulton, [0, 0, 0], _fultonAnchor, [0, 0, 0], _height];
		_fultonRope allowDamage false;

		private _fultonFunction = {
			params ["_object", "_objectName", "_mainFulton", "_currentFultonParticipants", "_participant"];
			_participant setVariable ["KH_var_fultonAttached", true, true];

			[
				[_object, _mainFulton, _currentFultonParticipants, _participant],
				{
					params ["_object", "_mainFulton", "_currentFultonParticipants", "_participant"];

					if (isNil "KH_var_fultonRespawnReset") then {
						KH_var_fultonRespawnReset = true;

						[
							"KH_eve_playerRespawned", 
							{
								params ["_unit"];
								_unit setVariable ["KH_var_fultonAttached", false, true];
								["KH_eve_fultonDetached", [], _unit] call CBA_fnc_targetEvent;
							}
						] call CBA_fnc_addEventHandler;		
					};

					private _currentParticipants = missionNamespace getVariable [_currentFultonParticipants, []];
					_currentParticipants pushBackUnique _participant;
					missionNamespace setVariable [_currentFultonParticipants, _currentParticipants, true];
					private _anchorEnd = createVehicle ["Land_Can_V2_F", _participant, [], 0, "CAN_COLLIDE"];

					[
						[_anchorEnd],
						{
							params ["_anchorEnd"];
							_anchorEnd hideObjectGlobal true;
						},
						"SERVER",
						true,
						false
					] call KH_fnc_execute;

					_anchorEnd allowDamage false;
					_anchorEnd attachTo [_participant, [0, 0, 0], "Spine3"];
					private _rope = ropeCreate [_mainFulton, [0, 0, 0], _anchorEnd, [0, 0, 0], 100];
					_rope allowDamage false;

					[
						[_object, _participant, _mainFulton, _currentFultonParticipants, _anchorEnd, _rope],
						{
							private _participant = _this select 1;
							private _currentFultonParticipants = _this select 3;
							private _anchorEnd = _this select 4;
							private _rope = _this select 5;
							private _currentParticipants = missionNamespace getVariable [_currentFultonParticipants, []];

							if (_participant in _currentParticipants) then {
								_currentParticipants deleteAt (_currentParticipants find _participant);
								missionNamespace setVariable [_currentFultonParticipants, _currentParticipants, true];
							};
							
							_participant setVariable ["KH_var_fultonAttached", false, true];
							ropeDestroy _rope;
							deleteVehicle _anchorEnd;

							if (isPlayer _participant) then {
								["KH_eve_fultonDetached", [], _participant] call CBA_fnc_targetEvent;
							};
						},
						true,
						{
							private _object = param [0];
							private _participant = param [1];
							private _mainFulton = param [2];
							private _rope = param [5];

							(
								!(isNull (objectParent _participant)) || 
								!(_participant getVariable ["KH_var_fultonAttached", false]) || 
								(isNull _object) || 
								(isNull _mainFulton) || 
								(isNull _rope) || 
								!(alive _participant) || 
								((ropeAttachedObjects _mainFulton) isEqualTo [])
							);
						},
						false
					] call KH_fnc_execute;
				},
				"SERVER",
				true,
				false
			] call KH_fnc_execute;

			if (isPlayer _participant) then {
				private _detachAction = [
					_participant,
					["Detach Self From Fulton ", _objectName] joinString "",
					"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
					"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
					"(_this getVariable ['KH_var_fultonAttached', false])",
					"(_caller getVariable ['KH_var_fultonAttached', false])",
					{},
					{},
					{
						_caller setVariable ["KH_var_fultonAttached", false, true];
						["KH_eve_fultonDetached", []] call CBA_fnc_localEvent;
					},
					{},
					[],
					3,
					0,
					false,
					false,
					false
				] call BIS_fnc_holdActionAdd; 

				[
					"CBA",
					"KH_eve_fultonDetached",
					[_detachAction],
					{
						_args params ["_detachAction"];
						[player, _detachAction] call BIS_fnc_holdActionRemove;
						[_handlerId] call KH_fnc_removeHandler;
					}
				] call KH_fnc_addEventHandler;
			};
		};

		[
			[
				_object,
				["Attach Self To Fulton ", _objectName] joinString "",
				"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
				"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
				["
					(((count (missionNamespace getVariable ['", _currentFultonParticipants, "', []])) <= ", _maximumParticipants, ") && 
					!(_this getVariable ['KH_var_fultonAttached', false]) && ((_this distance _target) < 6))
				"] joinString "",
				["
					(((count (missionNamespace getVariable ['", _currentFultonParticipants, "', []])) <= ", _maximumParticipants, ") && 
					!(_caller getVariable ['KH_var_fultonAttached', false]) && ((_caller distance _target) < 6))
				"] joinString "",
				{},
				{},
				{
					(_this select 3) params ["_objectName", "_mainFulton", "_currentFultonParticipants", "_fultonFunction"];
					[_target, _objectName, _mainFulton, _currentFultonParticipants, _caller] call _fultonFunction;
				},
				{},
				[_objectName, _mainFulton, _currentFultonParticipants, _fultonFunction],
				3,
				0,
				false,
				false,
				true
			],
			"BIS_fnc_holdActionAdd", 
			"PLAYERS",
			true,
			["JIP", _object, false, ""]
		] call KH_fnc_execute;

		[
			[_object, _maximumParticipants, _objectName, _mainFulton, _fultonActive, _currentFultonParticipants, _fultonFunction, _fultonRemoteActions, _fultonBox],
			{
				params ["_object", "_maximumParticipants", "_objectName", "_mainFulton", "_fultonActive", "_currentFultonParticipants", "_fultonFunction", "_fultonRemoteActions", "_fultonBox"];
				
				if !(missionNamespace getVariable [_fultonActive, false]) then {
					{
						if !(_x getVariable [_fultonRemoteActions, false]) then {
							_x setVariable [_fultonRemoteActions, true];

							[
								[
									_x,
									["Attach ", name _x, " To Fulton ", _objectName] joinString "",
									"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
									"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
									["
										(((count (missionNamespace getVariable ['", _currentFultonParticipants, "', []])) <= ", _maximumParticipants, ") && 
										!(_target getVariable ['KH_var_fultonAttached', false]) && ((_target distance (missionNamespace getVariable '", _fultonBox, "')) < 6) && 
										(_target isNotEqualTo _this) && ((_this distance _target) < 4) && (alive _target))
									"] joinString "",
									["
										(((count (missionNamespace getVariable ['", _currentFultonParticipants, "', []])) <= ", _maximumParticipants, ") && 
										!(_target getVariable ['KH_var_fultonAttached', false]) && ((_target distance (missionNamespace getVariable '", _fultonBox, "')) < 6) && 
										(_target isNotEqualTo _caller) && ((_caller distance _target) < 4) && (alive _target))
									"] joinString "",
									{},
									{},
									{
										(_this select 3) params ["_object", "_objectName", "_mainFulton", "_currentFultonParticipants", "_fultonFunction"];
										[_object, _objectName, _mainFulton, _currentFultonParticipants, _target] call _fultonFunction;
									},
									{},
									[_object, _objectName, _mainFulton, _currentFultonParticipants, _fultonFunction],
									3,
									0,
									false,
									false,
									true
								],
								"BIS_fnc_holdActionAdd", 
								"PLAYERS",
								true,
								["JIP", _x, false, ""]
							] call KH_fnc_execute;

							[
								[
									_x,
									["Detach ", name _x, " From Fulton ", _objectName] joinString "",
									"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
									"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_unloadVehicle_ca.paa",
									"((_target getVariable ['KH_var_fultonAttached', false]) && (_target isNotEqualTo _this) && ((_this distance _target) < 4) && (alive _target))",
									"((_target getVariable ['KH_var_fultonAttached', false]) && (_target isNotEqualTo _caller) && ((_caller distance _target) < 4) && (alive _target))",
									{},
									{},
									{
										_target setVariable ["KH_var_fultonAttached", false, true];

										if (isPlayer _target) then {
											["KH_eve_fultonDetached", [], _target] call CBA_fnc_targetEvent;
										};
									},
									{},
									[],
									3,
									0,
									false,
									false,
									true
								],
								"BIS_fnc_holdActionAdd",
								"PLAYERS",
								true,
								["JIP", "PLAYERS", _x, false, false, ""]
							] call KH_fnc_execute;
						};
					} forEach (_object nearEntities ["Man", 15]);
				}
				else {
					[_handle] call CBA_fnc_removePerFrameHandler;
				};
			},
			true,
			1,
			false
		] call KH_fnc_execute;

		[
			[_object, _vehicles, _height, _distance, _duration, _mainFulton, _fultonActive, _currentFultonParticipants, _mainFultonVelocity, _fultonAnchor, _fultonRope],
			{
				params ["_object", "_vehicles", "_height", "_distance", "_duration", "_mainFulton", "_fultonActive", "_currentFultonParticipants", "_mainFultonVelocity", "_fultonAnchor", "_fultonRope"];

				[
					[_object, _vehicles, _height, _distance, _duration, _mainFulton, _fultonActive, _currentFultonParticipants, _mainFultonVelocity, _fultonAnchor, _fultonRope],
					{
						params [
							"_object", 
							"_vehicles", 
							"_height", 
							"_distance", 
							"_duration", 
							"_mainFulton", 
							"_fultonActive", 
							"_currentFultonParticipants", 
							"_mainFultonVelocity", 
							"_fultonAnchor", 
							"_fultonRope"
						];
						
						if !(isNull _mainFulton) then {
							if (_vehicles isEqualTo []) then {
								_vehicles = _mainFulton nearEntities ["Plane", _distance + 100];
							}
							else {
								private _vehiclesNull = true;

								{
									if !(isNull _x) then {
										_vehiclesNull = false;
										break;
									};
								} forEach _vehicles;

								if _vehiclesNull then {
									_vehicles = _mainFulton nearEntities ["Plane", _distance + 100];
								};
							};

							{
								if ((_x distance _mainFulton) < _distance) then {
									private _vehicle = _x;
									
									{
										[
											[_x, _vehicle, _duration],
											{
												params ["_unit", "_vehicle", "_duration"];
												_unit setVariable ["KH_var_fultonAttached", false, true];
												_unit allowDamage false;
												_unit setUnitFreefallHeight 10000;

												[
													[_unit],
													{
														params ["_unit"];
														_unit switchMove ["Para_Pilot"];
													},
													"GLOBAL",
													true,
													false
												] call KH_fnc_execute;

												if (isPlayer _unit) then {
													addCamShake [2, _duration * 2, 18];
												};

												private _targetTime = CBA_missionTime + _duration;

												[
													[_unit, _vehicle, _targetTime],
													{
														params ["_unit", "_vehicle", "_targetTime"];

														[
															"MISSION",
															"Draw3D",
															[_unit, _vehicle, _targetTime],
															{
																_args params ["_unit", "_vehicle", "_targetTime"];

																if ((CBA_missionTime < _targetTime) && ((_unit distance _vehicle) >= 30)) then {
																	drawLine3D [
																		_unit modelToWorldVisual [0, 0, 0.5], 
																		_vehicle modelToWorldVisual [0, 0, 0], 
																		[0.8, 0.6, 0.4, 1],
																		15
																	]
																}
																else {
																	removeMissionEventHandler [_thisEvent, _thisEventHandler];
																};
															}
														] call KH_fnc_addEventHandler;
													},
													"GLOBAL",
													true,
													false
												] call KH_fnc_execute;

												[
													[_unit, _vehicle, _duration],
													{
														params ["_unit", "_vehicle", "_duration"];
														private _remainingTime = _unit getVariable ["KH_var_fultonRemainingTime", _duration];
														private _timeStep = diag_deltaTime;
														_remainingTime = _remainingTime - _timeStep;
														_unit setVariable ["KH_var_fultonRemainingTime", _remainingTime];

														if ((_remainingTime <= 0) || ((_unit distance _vehicle) < 30)) then {
															_unit switchMove [""];
															_unit moveInAny _vehicle;

															if ((objectParent _unit) isNotEqualTo _vehicle) then {
																private _timeout = CBA_missionTime + 15;

																[
																	[_unit, _vehicle, _timeout],
																	{
																		params ["_unit", "_vehicle", "_timeout"];
																		private _finished = false;

																		if ((objectParent _unit) isNotEqualTo _vehicle) then {
																			_unit moveInAny _vehicle;
																		}
																		else {
																			_finished = true;
																			_unit allowDamage true;
																			_unit setUnitFreefallHeight -1;
																			[_handlerId] call KH_fnc_removeHandler;
																		};

																		if ((CBA_missionTime > _timeout) && !_finished) then {
																			_unit allowDamage true;
																			_unit setUnitFreefallHeight -1;
																			[_handlerId] call KH_fnc_removeHandler;
																		};
																	},
																	true,
																	0,
																	false
																] call KH_fnc_execute;
															};

															_unit setVariable ["KH_var_fultonRemainingTime", nil];
															[_handlerId] call KH_fnc_removeHandler;
														} 
														else {
															if ((animationState _unit) isNotEqualTo "Para_Pilot") then {
																_unit switchMove ["Para_Pilot"];
															};

															private _unitPosition = getPosASL _unit;
															private _vehiclePosition = getPosASL _vehicle;
															private _vehicleRotation = getRotationEuler _vehicle;
															private _relativeRotation = _vehicle getRotationEuler _unit;
															private _velocity = (_vehiclePosition vectorDiff _unitPosition) vectorMultiply (3 / _remainingTime);
															_unit setRotationEuler [-(_relativeRotation select 0) - 75, 0, (_relativeRotation select 2) + 180];
															_unit setVelocity _velocity;
															_unit setAngularVelocity [0, 0, 0];
														};
													},
													true,
													0,
													false
												] call KH_fnc_execute;
											},
											_x,
											true,
											false
										] call KH_fnc_execute;
									} forEach (missionNamespace getVariable [_currentFultonParticipants, []]);

									missionNamespace setVariable [_fultonActive, true, true];
									ropeDestroy _fultonRope;
									deleteVehicle _fultonAnchor;
									deleteVehicle _mainFulton;
									deleteVehicle _object;
									[_handlerId] call KH_fnc_removeHandler;
									break;
								};
							} forEach _vehicles;
						}
						else {
							ropeDestroy _fultonRope;
							deleteVehicle _fultonAnchor;
							[_handlerId] call KH_fnc_removeHandler;
						};

						if !(missionNamespace getVariable [_fultonActive, false]) then {
							if (((getPosATL _mainFulton) select 2) < _height) then {
								missionNamespace setVariable [_mainFultonVelocity, (missionNamespace getVariable _mainFultonVelocity) + 0.02];
								_mainFulton setVelocity [0, 0, missionNamespace getVariable _mainFultonVelocity];
							}
							else {
								_mainFulton setVelocity [0, 0, 0];
							};
						};
					},
					true,
					0,
					false
				] call KH_fnc_execute;
			},
			true,
			"3",
			false
		] call KH_fnc_execute;
	}
] call KH_fnc_addEventHandler;