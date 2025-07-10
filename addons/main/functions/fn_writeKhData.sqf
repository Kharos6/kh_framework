params ["_file", "_variable", "_value", "_type", ["_overwrite", true]];
("kh_framework" callExtension ["WriteKHData", _this]) select 0;