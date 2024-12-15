params ["_object", "_vehicles", "_height", "_distance", "_maximumParticipants", "_duration", ["_objectName", ""]];
_height = _height min 100;
private _fultonEvent = format ["KH_eve_%1", [0, 36, "ALPHANUMERIC"] call KH_fnc_generateSymbols];
private _fultonId = [missionNamespace, "KH_var_fultonId", false, true] call KH_fnc_atomicVariable;

[
	[
		_object,
		format ["Prime Fulton %1", _objectName],
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
	["JIP", "PLAYERS", _object, false, false], 
	"THIS_FRAME"
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
		private _fultonActive = [missionNamespace, "KH_var_fultonActive", false, true] call KH_fnc_atomicVariable;
		private _currentFultonParticipants = [missionNamespace, "KH_var_currentFultonParticipants", [], true] call KH_fnc_atomicVariable;
		private _fultonBox = [missionNamespace, "KH_var_fultonBox", _object, true] call KH_fnc_atomicVariable;
		private _mainFultonVelocity = [missionNamespace, "KH_var_fultonExtractVelocity", 0.1, false] call KH_fnc_atomicVariable;
		private _fultonRemoteActions = [missionNamespace, "KH_var_fultonRemoteActions", false, false] call KH_fnc_atomicVariable;
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
						"THIS_FRAME"
					] call KH_fnc_execute;

					_anchorEnd allowDamage false;
					_anchorEnd attachTo [_participant, [0, 0, 0], "Spine3"];
					private _rope = ropeCreate [_mainFulton, [0, 0, 0], _anchorEnd, [0, 0, 0], 100];
					_rope allowDamage false;

					[
						{
							private _object = _this select 0;
							private _participant = _this select 1;
							private _mainFulton = _this select 2;
							private _rope = _this select 5;

							if (!(isNull (objectParent _participant)) || !(_participant getVariable ["KH_var_fultonAttached", false]) || (isNull _object) || (isNull _mainFulton) || (isNull _rope) || !(alive _participant) || ((ropeAttachedObjects _mainFulton) isEqualTo [])) then {
								true;
							}
							else {
								false;
							};
						}, 
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
						[_object, _participant, _mainFulton, _currentFultonParticipants, _anchorEnd, _rope]
					] call CBA_fnc_waitUntilAndExecute;
				},
				"SERVER",
				"THIS_FRAME"
			] call KH_fnc_execute;

			if (isPlayer _participant) then {
				private _detachAction = [
					_participant,
					format ["Detach Self From Fulton %1", _objectName],
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
						[_eventName, _localId] call CBA_fnc_removeEventHandler;
					}
				] call KH_fnc_addEventHandler;
			};
		};

		[
			[
				_object,
				format ["Attach Self To Fulton %1", _objectName],
				"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
				"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
				["(((count (missionNamespace getVariable ['", _currentFultonParticipants, "', []])) <= ", _maximumParticipants, ") && !(_this getVariable ['KH_var_fultonAttached', false]) && ((_this distance _target) < 6))"] joinString "",
				["(((count (missionNamespace getVariable ['", _currentFultonParticipants, "', []])) <= ", _maximumParticipants, ") && !(_caller getVariable ['KH_var_fultonAttached', false]) && ((_caller distance _target) < 6))"] joinString "",
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
			["JIP", "PLAYERS", _object, false, false], 
			"THIS_FRAME"
		] call KH_fnc_execute;

		[
			{
				_args params ["_object", "_maximumParticipants", "_objectName", "_mainFulton", "_fultonActive", "_currentFultonParticipants", "_fultonFunction", "_fultonRemoteActions", "_fultonBox"];
				
				if !(missionNamespace getVariable [_fultonActive, false]) then {
					{
						if !(_x getVariable [_fultonRemoteActions, false]) then {
							_x setVariable [_fultonRemoteActions, true];

							[
								[
									_x,
									format ["Attach %1 To Fulton %2", name _x, _objectName],
									"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
									"\a3\data_f_destroyer\data\UI\IGUI\Cfg\holdactions\holdAction_loadVehicle_ca.paa",
									["(((count (missionNamespace getVariable ['", _currentFultonParticipants, "', []])) <= ", _maximumParticipants, ") && !(_target getVariable ['KH_var_fultonAttached', false]) && ((_target distance (missionNamespace getVariable '", _fultonBox, "')) < 6) && (_target isNotEqualTo _this) && ((_this distance _target) < 4) && (alive _target))"] joinString "",
									["(((count (missionNamespace getVariable ['", _currentFultonParticipants, "', []])) <= ", _maximumParticipants, ") && !(_target getVariable ['KH_var_fultonAttached', false]) && ((_target distance (missionNamespace getVariable '", _fultonBox, "')) < 6) && (_target isNotEqualTo _caller) && ((_caller distance _target) < 4) && (alive _target))"] joinString "",
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
								["JIP", "PLAYERS", _x, false, false], 
								"THIS_FRAME"
							] call KH_fnc_execute;

							[
								[
									_x,
									format ["Detach %1 From Fulton %2", name _x, _objectName],
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
								["JIP", "PLAYERS", _x, false, false], 
								"THIS_FRAME"
							] call KH_fnc_execute;
						};
					} forEach (_object nearEntities ["Man", 15]);
				}
				else {
					[_handle] call CBA_fnc_removePerFrameHandler;
				};
			},
			1,
			[_object, _maximumParticipants, _objectName, _mainFulton, _fultonActive, _currentFultonParticipants, _fultonFunction, _fultonRemoteActions, _fultonBox]
		] call CBA_fnc_addPerFrameHandler;

		[
			{
				params ["_object", "_vehicles", "_height", "_distance", "_duration", "_mainFulton", "_fultonActive", "_currentFultonParticipants", "_mainFultonVelocity", "_fultonAnchor", "_fultonRope"];

				[
					{
						_args params ["_object", "_vehicles", "_height", "_distance", "_duration", "_mainFulton", "_fultonActive", "_currentFultonParticipants", "_mainFultonVelocity", "_fultonAnchor", "_fultonRope"];
						
						if !(isNull _mainFulton) then {
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
													"THIS_FRAME"
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

																if (!(CBA_missionTime >= _targetTime) && !((_unit distance _vehicle) < 30)) then {
																	drawLine3D [
																		_unit modelToWorld [0, 0, 0.5], 
																		_vehicle modelToWorld [0, 0, 0], 
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
													"THIS_FRAME"
												] call KH_fnc_execute;

												[
													{
														_args params ["_unit", "_vehicle", "_duration"];
														private _remainingTime = _unit getVariable ["KH_var_fultonRemainingTime", _duration];
														private _timeStep = diag_deltaTime;
														_remainingTime = _remainingTime - _timeStep;
														_unit setVariable ["KH_var_fultonRemainingTime", _remainingTime];

														if ((_remainingTime <= 0) || ((_unit distance _vehicle) < 30)) then {
															_unit switchMove [""];

															[
																{
																	params ["_unit", "_vehicle"];
																	_unit moveInAny _vehicle;

																	[
																		{
																			params ["_unit", "_vehicle"];

																			if ((objectParent _unit) != _vehicle) then {
																				_unit moveInAny _vehicle;
																				private _timeout = CBA_missionTime + 15;

																				[
																					{
																						_args params ["_unit", "_vehicle", "_timeout"];
																						private _finished = false;

																						if ((objectParent _unit) != _vehicle) then {
																							_unit moveInAny _vehicle;
																						}
																						else {
																							_finished = true;
																							_unit allowDamage true;
																							_unit setUnitFreefallHeight -1;
																							[_handle] call CBA_fnc_removePerFrameHandler;
																						};

																						if ((CBA_missionTime > _timeout) && !_finished) then {
																							_unit allowDamage true;
																							_unit setUnitFreefallHeight -1;
																							[_handle] call CBA_fnc_removePerFrameHandler;
																						};
																					},
																					0,
																					[_unit, _vehicle, _timeout]
																				] call CBA_fnc_addPerFrameHandler;
																			};
																		}, 
																		[_unit, _vehicle]
																	] call CBA_fnc_execNextFrame;
																}, 
																[_unit, _vehicle]
															] call CBA_fnc_execNextFrame;

															_unit setVariable ["KH_var_fultonRemainingTime", nil];
															[_handle] call CBA_fnc_removePerFrameHandler;
														} 
														else {
															if ((animationState _unit) != "Para_Pilot") then {
																_unit switchMove ["Para_Pilot"];
															};

															private _unitPosition = getPosATL _unit;
															private _vehiclePosition = getPosATL _vehicle;
															private _vehicleRotation = [_vehicle, objNull] call KH_fnc_getRotation;
															private _relativeRotation = [_unit, _vehicle] call KH_fnc_getRotation;
															private _velocity = (_vehiclePosition vectorDiff _unitPosition) vectorMultiply (3 / _remainingTime);
															[_unit, [-(_relativeRotation select 0) - 75, 0, (_relativeRotation select 2) + 180], false] call KH_fnc_setRotation;
															_unit setVelocity _velocity;
															_unit setAngularVelocity [0, 0, 0];
														};
													},
													0,
													[_unit, _vehicle, _duration]
												] call CBA_fnc_addPerFrameHandler;
											},
											_x,
											"THIS_FRAME"
										] call KH_fnc_execute;
									} forEach (missionNamespace getVariable [_currentFultonParticipants, []]);

									missionNamespace setVariable [_fultonActive, true, true];
									ropeDestroy _fultonRope;
									deleteVehicle _fultonAnchor;
									deleteVehicle _mainFulton;
									deleteVehicle _object;
									[_handle] call CBA_fnc_removePerFrameHandler;
								};
							} forEach _vehicles;
						}
						else {
							ropeDestroy _fultonRope;
							deleteVehicle _fultonAnchor;
							[_handle] call CBA_fnc_removePerFrameHandler;
						};

						if !(missionNamespace getVariable [_fultonActive, false]) then {
							if (((getPosATL _mainFulton) select 2) < _height) then {
								private _velocity = [missionNamespace, _mainFultonVelocity, 0.02, false] call KH_fnc_atomicValue;
								_mainFulton setVelocity [0, 0, _velocity];
							}
							else {
								_mainFulton setVelocity [0, 0, 0];
							};
						};
					},
					0,
					[_object, _vehicles, _height, _distance, _duration, _mainFulton, _fultonActive, _currentFultonParticipants, _mainFultonVelocity, _fultonAnchor, _fultonRope]
				] call CBA_fnc_addPerFrameHandler;
			}, 
			[_object, _vehicles, _height, _distance, _duration, _mainFulton, _fultonActive, _currentFultonParticipants, _mainFultonVelocity, _fultonAnchor, _fultonRope], 
			3
		] call CBA_fnc_waitAndExecute;
	}
] call KH_fnc_addEventHandler;

true;