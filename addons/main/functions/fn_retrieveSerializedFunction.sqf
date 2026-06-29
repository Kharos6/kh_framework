params [["_function", "", [""]]];
private _storedFunction = missionNamespace getVariable _function;
[[], [_storedFunction]] select !(isNil "_storedFunction");