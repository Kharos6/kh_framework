params [["_type", "", [""]], ["_arguments", [], [[]]], ["_duration", false, [true, 0, ""]], "_conditionArguments", ["_conditionFunction", {}, ["", {}]]];

if (_duration isEqualTo false) then {
    _duration = 0;
};

switch _type do {
    case "LINE": {
        _arguments params [
            ["_start", [], [[], objNull]], 
            ["_end", [], [[], objNull]], 
            ["_color", [1, 1, 1, 1], [[]]], 
            ["_width", 10, [0]]
        ];

        if (_duration isEqualTo true) then {
            if (_start isEqualType objNull) then {
                _start = _start modelToWorldVisual [0, 0, 0];
            }
            else {
                if ((_start select 0) isEqualType objNull) then {
                    _start = (_start select 0) modelToWorldVisual (_start select 1);
                };
            };

            if (_end isEqualType objNull) then {
                _end = _end modelToWorldVisual [0, 0, 0];
            }
            else {
                if ((_end select 0) isEqualType objNull) then {
                    _end = (_end select 0) modelToWorldVisual (_end select 1);
                };
            };

            drawLine3D [_start, _end, _color, _width];
        }
        else {
            private _typeObject = if (_start isEqualType objNull) then {
                true;
            }
            else {
                if (_end isEqualType objNull) then {
                    true;
                }
                else {
                    if ((_start select 0) isEqualType objNull) then {
                        true;
                    }
                    else {
                        [false, true] select ((_end select 0) isEqualType objNull);
                    };
                };
            };

            if _typeObject then {
                [
                    ["DRAW_UI", _conditionArguments, _conditionFunction, _duration, [], {}],
                    true,
                    [_start, _end, _color, _width],
                    {
                        _args params ["_start", "_end", "_color", "_width"];

                        if (_start isEqualType objNull) then {
                            _start = _start modelToWorldVisual [0, 0, 0];
                        }
                        else {
                            if ((_start select 0) isEqualType objNull) then {
                                _start = (_start select 0) modelToWorldVisual (_start select 1);
                            };
                        };

                        if (_end isEqualType objNull) then {
                            _end = _end modelToWorldVisual [0, 0, 0];
                        }
                        else {
                            if ((_end select 0) isEqualType objNull) then {
                                _end = (_end select 0) modelToWorldVisual (_end select 1);
                            };
                        };

                        drawLine3D [_start, _end, _color, _width];
                    }
                ] call KH_fnc_addEventHandler;
            }
            else {
                [
                    ["DRAW_UI", _conditionArguments, _conditionFunction, _duration, [], {}],
                    true,
                    [_start, _end, _color, _width],
                    {
                        drawLine3D _args;
                    }
                ] call KH_fnc_addEventHandler;
            };
        };
    };

    case "LASER": {
        _arguments params [
            ["_start", [], [[], objNull]], 
            ["_end", [], [[], objNull]], 
            ["_beamColor", [1, 1, 1], [[]]], 
            ["_dotColor", [1, 1, 1], [[]]], 
            ["_dotSize", 0, [0]], 
            ["_beamThickness", 1, [0]],
            ["_beamMaxLength", 0, [0]], 
            ["_ir", false, [true]]
        ];

        if (_duration isEqualTo true) then {
            if (_start isEqualType objNull) then {
                _start = _start modelToWorldVisualWorld [0, 0, 0];
            }
            else {
                if ((_start select 0) isEqualType objNull) then {
                    _start = (_start select 0) modelToWorldVisualWorld (_start select 1);
                };
            };

            _end = if (_end isEqualType objNull) then {
                _start vectorFromTo (_end modelToWorldVisualWorld [0, 0, 0]);
            }
            else {
                if ((_end select 0) isEqualType objNull) then {
                    _start vectorFromTo ((_end select 0) modelToWorldVisualWorld (_end select 1));
                }
                else {
                    _start vectorFromTo _end;
                };
            };

            if (_beamMaxLength isEqualTo 0) then {
                _beamMaxLength = _start vectorDistance _end;
            };

            drawLaser [_start, _end, _beamColor, _dotColor, _dotSize, _beamThickness, _beamMaxLength, _ir];
        }
        else {
            private _typeObject = if (_start isEqualType objNull) then {
                true;
            }
            else {
                if (_end isEqualType objNull) then {
                    true;
                }
                else {
                    if ((_start select 0) isEqualType objNull) then {
                        true;
                    }
                    else {
                        [false, true] select ((_end select 0) isEqualType objNull);
                    };
                };
            };

            if _typeObject then {
                [
                    ["DRAW_UI", [], {}, _duration, [], {}],
                    true,
                    [_start, _end, _beamColor, _dotColor, _dotSize, _beamThickness, _beamMaxLength, _ir],
                    {
                        _args params ["_start", "_end", "_beamColor", "_dotColor", "_dotSize", "_beamThickness", "_beamMaxLength", "_ir"];

                        if (_start isEqualType objNull) then {
                            _start = _start modelToWorldVisualWorld [0, 0, 0];
                        }
                        else {
                            if ((_start select 0) isEqualType objNull) then {
                                _start = (_start select 0) modelToWorldVisualWorld (_start select 1);
                            };
                        };

                        _end = if (_end isEqualType objNull) then {
                            _start vectorFromTo (_end modelToWorldVisualWorld [0, 0, 0]);
                        }
                        else {
                            if ((_end select 0) isEqualType objNull) then {
                                _start vectorFromTo ((_end select 0) modelToWorldVisualWorld (_end select 1));
                            }
                            else {
                                _start vectorFromTo _end;
                            };
                        };

                        if (_beamMaxLength isEqualTo 0) then {
                            _beamMaxLength = _start vectorDistance _end;
                        };

                        drawLaser [_start, _end, _beamColor, _dotColor, _dotSize, _beamThickness, _beamMaxLength, _ir];
                    }
                ] call KH_fnc_addEventHandler;
            }
            else {
                if (_beamMaxLength isEqualTo 0) then {
                    _beamMaxLength = _start vectorDistance _end;
                };

                _end = _start vectorFromTo _end;

                [
                    ["DRAW_UI", [], {}, _duration, [], {}],
                    true,
                    [_start, _end, _beamColor, _dotColor, _dotSize, _beamThickness, _beamMaxLength, _ir],
                    {
                        drawLaser _args;
                    }
                ] call KH_fnc_addEventHandler;
            };
        };
    };

    case "ICON": {
        _arguments params [
            ["_texture", "\a3\ui_f\data\IGUI\Cfg\Radar\radar_ca.paa", [""]],
            ["_color", [1, 1, 1, 1], [[]]],
            ["_position", [0, 0, 0], [[], objNull]],
            ["_width", 1, [0]],
            ["_height", 1, [0]],
            ["_angle", 1, [0]],
            ["_text", "", [""]],
            ["_shadow", false, [true, 0]],
            ["_textSize", 1, [0]],
            ["_font", "RobotoCondensedBold", [""]],
            ["_textAlign", "center", [""]],
            ["_drawSideArrows", false, [true]],
            ["_offsetX", 0, [0]],
            ["_offsetY", 0, [0]]
        ];

        if (_duration isEqualTo true) then {
            if (_position isEqualType objNull) then {
                _position = _position modelToWorldVisual [0, 0, 0];
            }
            else {
                if ((_position select 0) isEqualType objNull) then {
                    _position = (_position select 0) modelToWorldVisual (_position select 1);
                };
            };

            if (!_drawSideArrows && ((worldToScreen _position) isEqualTo [])) exitWith {};
            drawIcon3D [_texture, _color, _position, _width, _height, _angle, _text, _shadow, _textSize, _font, _textAlign, _drawSideArrows, _offsetX, _offsetY];
        }
        else {
            private _typeObject = if (_position isEqualType objNull) then {
                true;
            }
            else {
                [false, true] select ((_position select 0) isEqualType objNull);
            };

            if _typeObject then {
                [
                    ["DRAW_UI", _conditionArguments, _conditionFunction, _duration, [], {}],
                    true,
                    [_texture, _color, _position, _width, _height, _angle, _text, _shadow, _textSize, _font, _textAlign, _drawSideArrows, _offsetX, _offsetY],
                    {
                        _args params ["_texture", "_color", "_position", "_width", "_height", "_angle", "_text", "_shadow", "_textSize", "_font", "_textAlign", "_drawSideArrows", "_offsetX", "_offsetY"];

                        if (_position isEqualType objNull) then {
                            _position = _start modelToWorldVisual [0, 0, 0];
                        }
                        else {
                            if ((_position select 0) isEqualType objNull) then {
                                _position = (_position select 0) modelToWorldVisual (_position select 1);
                            };
                        };

                        if (!_drawSideArrows && ((worldToScreen _position) isEqualTo [])) exitWith {};
                        drawIcon3D [_texture, _color, _position, _width, _height, _angle, _text, _shadow, _textSize, _font, _textAlign, _drawSideArrows, _offsetX, _offsetY];
                    }
                ] call KH_fnc_addEventHandler;
            }
            else {
                [
                    ["DRAW_UI", _conditionArguments, _conditionFunction, _duration, [], {}],
                    true,
                    [_texture, _color, _position, _width, _height, _angle, _text, _shadow, _textSize, _font, _textAlign, _drawSideArrows, _offsetX, _offsetY],
                    {
                        if (!_drawSideArrows && ((worldToScreen _position) isEqualTo [])) exitWith {};
                        drawLine3D _args;
                    }
                ] call KH_fnc_addEventHandler;
            };
        };
    };

    default {
        nil;
    };
};