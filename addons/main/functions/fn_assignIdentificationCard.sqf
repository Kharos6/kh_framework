params ["_unit", "_name", "_gender", "_race", "_dateOfBirth", "_profession", "_ethnos", "_cardNumber", "_dateOfIssue", "_dateOfExpiry"];

if (isNil "KH_var_assignIdentificationCardSet") then {
	KH_var_assignIdentificationCardSet = true;

	[
		"KH_eve_playerRespawned",
		{
			private _newEntity = _this select 0;
			private _oldEntity = _this select 2;

			if !(isNil {_oldEntity getVariable 'KH_var_identificationCardInformation'}) then {
				[
					[_newEntity],
					{
						params ["_newEntity"];

						if (_newEntity != player) then {
							_newEntity addAction [
								"View Identification Card",
								{
									params ["_target"];
									(_target getVariable ["KH_var_identificationCardInformation", []]) call KH_fnc_openIdentificationCard;
								},
								nil,
								1.5,
								false,
								true,
								"",
								"true",
								4,
								false,
								"",
								""
							];
						}
						else {
							_newEntity addAction [
								"View Own Identification Card",
								{
									params ["_target"];
									(_target getVariable ["KH_var_identificationCardInformation", []]) call KH_fnc_openIdentificationCard;
								},
								nil,
								1.5,
								false,
								true,
								"",
								"true",
								4,
								false,
								"",
								""
							];
						};
					},
					["JIP", "PLAYERS", _newEntity, true, false, ""], 
					"THIS_FRAME"
				] call KH_fnc_execute;
			};
		}
	] call CBA_fnc_addEventHandler;
};

if (_name == "") then {
	_name = name _unit;
};

if (_gender == "") then {
	_gender = "Male";
};

if (_race == "") then {
	private _face = face _unit;
	private _raceSelection = "";

	switch true do {
		case ("asian" in _face): {
			_raceSelection = "Asian";
		};

		case ("white" in _face): {
			_raceSelection = "Caucasian";
		};

		case (("tanoan" in _face) || ("african" in _face)): {
			_raceSelection = "African";
		};

		case ("russian" in _face): {
			_raceSelection = "Slavic";
		};

		default {
			_raceSelection = "Caucasian";
		};
	};

	_race = _raceSelection;
};

if (_dateOfBirth == "") then {
	private _day = ((floor (random 29)) max 1) min 28;

	if (_day < 10) then {
		_day = format ["0%1", _day];
	}
	else {
		_day = str _day;
	};

	private _month = ((floor (random 13)) max 1) min 12;

	if (_month < 10) then {
		_month = format ["0%1", _month];
	}
	else {
		_month = str _month;
	};

	private _year = floor (random [(date select 0) - 70, (date select 0) - 45, (date select 0) - 20]);
	_dateOfBirth = format ["%1/%2/%3", _day, _month, _year];
};

if (_profession == "") then {
	_profession = selectRandom [
		"Librarian",
		"Forensic Pathologist",
		"Marine Biologist",
		"Ice Cream Taster",
		"Corporate Spy",
		"Snake Milker",
		"Professional Bridesmaid",
		"Crime Scene Cleaner",
		"Elevator Mechanic",
		"Ethical Hacker",
		"Storm Chaser",
		"Airplane Repo Agent",
		"AI Ethicist",
		"Popcorn Flavor Developer",
		"Space Lawyer",
		"Mattress Tester",
		"Meme Curator",
		"Mortuary Makeup Artist",
		"Food Stylist",
		"Dog Surfing Instructor",
		"Ostrich Rancher",
		"Language Translator",
		"Systems Analyst",
		"Witch Investigator",
		"Greeting Card Writer",
		"Data Scientist",
		"Pigeon Racing Coordinator",
		"Underwater Welder",
		"Bubblegum Scientist",
		"Ghostwriter",
		"Rock Climbing Instructor",
		"Water Slide Tester",
		"Cartoon Voice Actor",
		"Drone Pilot",
		"Museum Curator",
		"Pyrotechnician",
		"Snake Venom Extractor",
		"Urban Planner",
		"Fermentation Scientist",
		"Toy Designer",
		"Sommelier",
		"Professional Cuddler",
		"Hazardous Materials Removal Worker",
		"Theme Park Mascot",
		"Rideshare Driver",
		"Airline Pilot",
		"Skyscraper Window Cleaner",
		"Movie Critic",
		"Tattoo Artist",
		"Bomb Squad Technician",
		"Stunt Double",
		"Firefighter",
		"Personal Shopper",
		"Astronomer",
		"Whale-Watching Guide",
		"Chocolatier",
		"Magician",
		"Beer Taster",
		"Book Editor",
		"Opera Singer",
		"Social Worker",
		"Feng Shui Consultant",
		"Carpenter",
		"Roadkill Removal Specialist",
		"Toy Tester",
		"Crime Scene Photographer",
		"VR Game Developer",
		"Beekeeper",
		"Event Planner",
		"Automotive Mechanic",
		"Glazier (Window Glass Installer)",
		"Midwife",
		"Horse Trainer",
		"Real Estate Agent",
		"Paralegal",
		"Political Speechwriter",
		"Headphone Untangler",
		"Food Critic",
		"Paranormal Investigator",
		"Mosaic Artist",
		"Professional Mermaid",
		"Puppeteer",
		"Toy Collector Consultant",
		"Quilter",
		"Headhunter",
		"Personal Chef",
		"Foreign Correspondent",
		"Etiquette Coach",
		"Stress Management Coach",
		"Cloud Architect",
		"Zookeeper",
		"Etymologist",
		"Ornithologist",
		"Drone Racing Commentator",
		"Cartoonist",
		"Furniture Tester",
		"Ball Pit Cleaner",
		"Candle Maker",
		"Haunted House Actor",
		"Psychic Hotline Operator"
	];
};

if (_ethnos == "") then {
	_ethnos = selectRandom [
		"Afghan",
		"Albanian",
		"Algerian",
		"American",
		"Andorran",
		"Angolan",
		"Antiguan or Barbudan",
		"Argentine",
		"Armenian",
		"Australian",
		"Austrian",
		"Azerbaijani",
		"Bahamian",
		"Bahraini",
		"Bangladeshi",
		"Barbadian",
		"Belarusian",
		"Belgian",
		"Belizean",
		"Beninese",
		"Bhutanese",
		"Bolivian",
		"Bosnian or Herzegovinian",
		"Botswanan",
		"Brazilian",
		"British",
		"Bruneian",
		"Bulgarian",
		"Burkinabé",
		"Burmese",
		"Burundian",
		"Cabo Verdean",
		"Cambodian",
		"Cameroonian",
		"Canadian",
		"Central African",
		"Chadian",
		"Chilean",
		"Chinese",
		"Colombian",
		"Comoran",
		"Congolese (Republic of the Congo)",
		"Congolese (Democratic Republic of the Congo)",
		"Costa Rican",
		"Croatian",
		"Cuban",
		"Cypriot",
		"Czech",
		"Danish",
		"Djiboutian",
		"Dominican",
		"Dutch",
		"East Timorese",
		"Ecuadorian",
		"Egyptian",
		"Emirati",
		"Equatorial Guinean",
		"Eritrean",
		"Estonian",
		"Swazi",
		"Ethiopian",
		"Micronesian",
		"Fijian",
		"Finnish",
		"French",
		"Gabonese",
		"Gambian",
		"Georgian",
		"German",
		"Ghanaian",
		"Greek",
		"Grenadian",
		"Guatemalan",
		"Guinean",
		"Guinea-Bissauan",
		"Guyanese",
		"Haitian",
		"Honduran",
		"Hungarian",
		"Icelandic",
		"Indian",
		"Indonesian",
		"Iranian",
		"Iraqi",
		"Irish",
		"Israeli",
		"Italian",
		"Ivorian",
		"Jamaican",
		"Japanese",
		"Jordanian",
		"Kazakh",
		"Kenyan",
		"Kittitian or Nevisian",
		"Kosovar",
		"Kuwaiti",
		"Kyrgyz",
		"Laotian",
		"Latvian",
		"Lebanese",
		"Basotho",
		"Liberian",
		"Libyan",
		"Liechtensteiner",
		"Lithuanian",
		"Luxembourger",
		"Macedonian",
		"Malagasy",
		"Malawian",
		"Malaysian",
		"Maldivian",
		"Malian",
		"Maltese",
		"Marshallese",
		"Mauritanian",
		"Mauritian",
		"Mexican",
		"Moldovan",
		"Monacan",
		"Mongolian",
		"Montenegrin",
		"Moroccan",
		"Mozambican",
		"Namibian",
		"Nauruan",
		"Nepalese",
		"New Zealander",
		"Nicaraguan",
		"Nigerien",
		"Nigerian",
		"Ni-Vanuatu",
		"North Korean",
		"Norwegian",
		"Omani",
		"Pakistani",
		"Palauan",
		"Palestinian",
		"Panamanian",
		"Papua New Guinean",
		"Paraguayan",
		"Peruvian",
		"Filipino",
		"Polish",
		"Portuguese",
		"Qatari",
		"Romanian",
		"Russian",
		"Rwandan",
		"Saint Lucian",
		"Saint Vincentian",
		"Sammarinese",
		"Samoan",
		"São Toméan",
		"Saudi Arabian",
		"Senegalese",
		"Serbian",
		"Seychellois",
		"Sierra Leonean",
		"Singaporean",
		"Slovak",
		"Slovenian",
		"Solomon Islander",
		"Somali",
		"South African",
		"South Korean",
		"South Sudanese",
		"Spanish",
		"Sri Lankan",
		"Sudanese",
		"Surinamese",
		"Swazi",
		"Swedish",
		"Swiss",
		"Syrian",
		"Taiwanese",
		"Tajik",
		"Tanzanian",
		"Thai",
		"Togolese",
		"Tongan",
		"Trinidadian or Tobagonian",
		"Tunisian",
		"Turkish",
		"Turkmen",
		"Tuvaluan",
		"Ugandan",
		"Ukrainian",
		"Uruguayan",
		"Uzbek",
		"Vatican",
		"Venezuelan",
		"Vietnamese",
		"Yemeni",
		"Zambian",
		"Zimbabwean"
	];
};

if (_cardNumber == "") then {
	_cardNumber = [12, true, false, false] call KH_fnc_generateRandomString;
};

if (_dateOfIssue == "") then {
	private _day = ((floor (random 29)) max 1) min 28;

	if (_day < 10) then {
		_day = format ["0%1", _day];
	}
	else {
		_day = str _day;
	};

	private _month = ((floor (random 13)) max 1) min 12;

	if (_month < 10) then {
		_month = format ["0%1", _month];
	}
	else {
		_month = str _month;
	};

	private _year = floor (random [(date select 0) - 8, (date select 0) - 3, (date select 0) - 1]);
	_dateOfIssue = format ["%1/%2/%3", _day, _month, _year];
};

if (_dateOfExpiry == "") then {
	private _year = (parseNumber (_dateOfIssue select [6])) + 5;
	_dateOfExpiry = format ["%1%2", _dateOfIssue select [0, 6], _year];
};

_unit setVariable ["KH_var_identificationCardInformation", [_name, _gender, _race, _dateOfBirth, _profession, _ethnos, _cardNumber, _dateOfIssue, _dateOfExpiry], true];

[
	[_unit],
	{
		params ["_unit"];

		if (_unit != player) then {
			_unit addAction [
				"View Identification Card",
				{
					params ["_target"];
					(_target getVariable ["KH_var_identificationCardInformation", []]) call KH_fnc_openIdentificationCard;
				},
				nil,
				1.5,
				false,
				true,
				"",
				"true",
				4,
				false,
				"",
				""
			];
		}
		else {
			_unit addAction [
				"View Own Identification Card",
				{
					params ["_target"];
					(_target getVariable ["KH_var_identificationCardInformation", []]) call KH_fnc_openIdentificationCard;
				},
				nil,
				1.5,
				false,
				true,
				"",
				"true",
				4,
				false,
				"",
				""
			];
		};
	},
	["JIP", "PLAYERS", _unit, true, false, ""], 
	"THIS_FRAME"
] call KH_fnc_execute;

true;