[
	selectRandom ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"], 
	("kh_framework" callExtension ["GenerateRandomString", [16, true, true, false]]) select 0, 
	clientOwner
] joinString "_";