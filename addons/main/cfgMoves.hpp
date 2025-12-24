class CfgMovesBasic
{
    class States;
};
class CfgMovesMaleSdr: CfgMovesBasic
{
    class States: States
    {
        class AmovPknlMstpSrasWlnrDnon_AinvPknlMstpSrasWlnrDnon;
        class AinvPknlMstpSlayWlnrDnon_medicOther: AmovPknlMstpSrasWlnrDnon_AinvPknlMstpSrasWlnrDnon
        {
            forceAim = 1;
        };
        class AmovPknlMstpSnonWnonDnon;
        class AinvPknlMstpSlayWnonDnon_medic: AmovPknlMstpSnonWnonDnon
        {
            forceAim = 1;
        };
        class AmovPknlMstpSrasWrflDnon_AinvPknlMstpSlayWrflDnon;
        class AinvPknlMstpSlayWnonDnon_medicOther: AmovPknlMstpSrasWrflDnon_AinvPknlMstpSlayWrflDnon
        {
            forceAim = 1;
        };
        class AmovPknlMstpSrasWpstDnon;
        class AinvPknlMstpSlayWpstDnon_medic: AmovPknlMstpSrasWpstDnon
        {
            forceAim = 1;
        };
        class AinvPknlMstpSlayWpstDnon_medicOther: AmovPknlMstpSrasWpstDnon
        {
            forceAim = 1;
        };
        class AinvPknlMstpSlayWrflDnon_medic: AmovPknlMstpSrasWrflDnon_AinvPknlMstpSlayWrflDnon
        {
            forceAim = 1;
        };
        class AinvPknlMstpSlayWrflDnon_medicOther: AmovPknlMstpSrasWrflDnon_AinvPknlMstpSlayWrflDnon
        {
            forceAim = 1;
        };
        class AinvPpneMstpSlayWnonDnon_medic: AinvPknlMstpSlayWpstDnon_medic
        {
            forceAim = 1;
        };
        class AinvPpneMstpSlayWnonDnon_medicOther: AinvPknlMstpSlayWnonDnon_medicOther
        {
            forceAim = 1;
        };
        class AinvPpneMstpSlayWpstDnon_medic: AinvPknlMstpSlayWpstDnon_medic
        {
            forceAim = 1;
        };
        class AinvPpneMstpSlayWpstDnon_medicOther: AinvPknlMstpSlayWpstDnon_medicOther
        {
            forceAim = 1;
        };
        class AinvPpneMstpSlayWrflDnon_medic: AinvPknlMstpSlayWrflDnon_medic
        {
            forceAim = 1;
        };
        class AinvPpneMstpSlayWrflDnon_medicOther: AinvPknlMstpSlayWrflDnon_medic
        {
            forceAim = 1;
        };
    };
};