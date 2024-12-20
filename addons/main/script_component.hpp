#include "\a3\3den\ui\macros.inc"
#include "\a3\3den\ui\resincl.inc"
#include "\a3\ui_f\hpp\definecommongrids.inc"
#include "\a3\ui_f\hpp\definedikcodes.inc"
#include "\a3\ui_f\hpp\defineresincl.inc"
#include "\a3\ui_f\hpp\defineresincldesign.inc"
#include "\x\cba\addons\main\script_macros_common.hpp"
#define CTRL_DEFAULT_H (SIZE_M * GRID_H)
#define CTRL_DEFAULT_Y (pixelH)
#define CTRL_DEFAULT_X (ATTRIBUTE_TITLE_W * GRID_W)
#define CTRL_DEFAULT_W (82 * GRID_W)
#define RSC_POS_X(N) ((N) * GUI_GRID_W + GUI_GRID_CENTER_X)
#define RSC_POS_Y(N) ((N) * GUI_GRID_H + GUI_GRID_CENTER_Y)
#define RSC_POS_W(N) ((N) * GUI_GRID_W)
#define RSC_POS_H(N) ((N) * GUI_GRID_H)
#define GUI_THEME_RGB_R "(profileNamespace getVariable ['GUI_BCG_RGB_R', 0.13])"
#define GUI_THEME_RGB_G "(profileNamespace getVariable ['GUI_BCG_RGB_G', 0.54])"
#define GUI_THEME_RGB_B "(profileNamespace getVariable ['GUI_BCG_RGB_B', 0.21])"
#define GUI_THEME_ALPHA "(profileNamespace getVariable ['GUI_BCG_RGB_A', 0.8])"
class RscButtonMenuCancel;
class RscButtonMenuOK;
class RscCheckbox;
class RscText;
class ctrlButton;
class ctrlButtonCancel;
class ctrlButtonClose;
class ctrlButtonCollapseAll;
class ctrlButtonExpandAll;
class ctrlButtonOK;
class ctrlButtonPictureKeepAspect;
class ctrlButtonSearch;
class ctrlButtonToolbar;
class ctrlCheckbox;
class ctrlCheckboxToolbar;
class ctrlCombo;
class ctrlControlsGroup;
class ctrlControlsGroupNoScrollbars;
class ctrlEdit;
class ctrlEditMulti;
class ctrlListbox;
class ctrlListNBox;
class ctrlMap;
class ctrlMenu;
class ctrlMenuStrip;
class ctrlProgress;
class ctrlShortcutButtonSteam;
class ctrlStatic;
class ctrlStaticBackground;
class ctrlStaticBackgroundDisable;
class ctrlStaticBackgroundDisableTiles;
class ctrlStaticFooter;
class ctrlStaticFrame;
class ctrlStaticMulti;
class ctrlStaticOverlay;
class ctrlStaticPicture;
class ctrlStaticPictureKeepAspect;
class ctrlStaticTitle;
class ctrlStructuredText;
class ctrlToolbox;
class ctrlToolboxPictureKeepAspect;
class ctrlTree;
class ctrlXSliderH;
class scrollbar;