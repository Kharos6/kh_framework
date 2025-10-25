class RscButton;
class RscButtonMenuCancel;
class RscButtonMenuOK;
class RscCheckbox;
class RscCombo;
class RscEdit;
class RscEditMulti;
class RscListBox;
class RscListBoxKeys;
class RscListBoxMulti;
class RscListNBox;
class RscPicture;
class RscSlider;
class RscStructuredText;
class RscText;
class RscTextMulti;
class RscToolbox;
class RscTree;
class RscTreeMulti;
class RscVideo;
class KH_ResourcePicture: RscPicture
{
    x = QUOTE(RSC_POS_X(0));
    y = QUOTE(RSC_POS_Y(0));
    w = QUOTE(RSC_POS_W(100));
    h = QUOTE(RSC_POS_H(100));
};
class KH_ResourceVideo: RscVideo
{
    x = QUOTE(RSC_POS_X(0));
    y = QUOTE(RSC_POS_Y(0));
    w = QUOTE(RSC_POS_W(100));
    h = QUOTE(RSC_POS_H(100));
};
class RscTitles
{
    #include "\x\kh\addons\main\resources\khDisplay.hpp"
};
#include "\x\kh\addons\main\resources\identificationCard.hpp"
#include "\x\kh\addons\main\resources\debugConsole.hpp"
#include "\x\kh\addons\main\resources\khControl.hpp"
#include "\x\kh\addons\main\resources\terminal.hpp"