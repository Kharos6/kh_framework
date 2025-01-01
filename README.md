<h1 align="center">
  DESCRIPTION
</h1>

Tool package designed to facilitate the creation of missions and addons. Contains a library of functions, editor attributes, events, modules, and various global variables. Created out of necessity to make mission making easier for my unit, especially for mission makers who may not have much experience in coding. Granted, basic syntax knowledge is still required for certain functionalities. A good portion of this mod also exposes or simplifies processes that would otherwise involve tediously wrestling with the volatility of things like locality, remote execution, player unit existence, or JIP; all of which, and more, can now be solved with a single function or Eden attribute. Most notably, remote execution is now possible from any machine to any machine without the use of <remoteExec> or <remoteExecCall> thanks to <KH_fnc_execute>, which uses CBA events to achieve the same, and allows for even more intricate execution parameters. Finally, all code introduced by this mod runs in the unscheduled environment.

<h1 align="center">
  COMPATIBILITY
</h1>

* ace: All functions are designed to be compatible with ace, and will adjust their behaviour depending on whether ace is loaded or not.
* other mods: Likely compatible with just about everything else on the workshop, as this mod does not overwrite any original game behaviour or the behaviour of other mods. Explicitly tested with 3den Enhanced and Zeus Enhanced, with no incompatibilities found.

<h1 align="center">
  NOTES
</h1>

- While the large majority of the functionality exposed in the form of Eden attributes has been thoroughly tested, there is still a chance that some bugs flew under the radar. Reporting these issues is greatly appreciated.
- Due to multiplayer being the primary focus of this mod, testing missions in multiplayer mode is ideal, as singleplayer will misbehave in some circumstances. This may be changed in the future to allow for singleplayer mission creation, but is not a priority right now.
- Updates will be pushed based on volume or necessity. If major issues are found, an update will be pushed out to fix those issues as soon as possible. Otherwise, updates will be pushed out only when enough additions have been made to the mod to warrant an update.

<h1 align="center">
  DISCLAIMER
</h1>

So long as it is done in good will, and credit to the original author is provided alongside the necessary links to the original content, you are allowed to modify, redistribute, and repurpose this mod as you see fit.

<h1 align="center">
  CONTRIBUTION
</h1>

Anyone is welcome to contribute to this project.
