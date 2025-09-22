params [["_hashType", "", [""]], "_value"];

if !(_value isEqualType "") then {
    _value = (["", _value] call KH_fnc_serializeValue) select 1;
};

["kh_framework", [_hashType, _value], "CryptoOperation", false] call KH_fnc_callExtension;