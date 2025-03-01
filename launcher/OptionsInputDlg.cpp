#include <wxx_wincore.h>
#include "OptionsInputDlg.h"
#include "LauncherApp.h"
#include <wxx_commondlg.h>

OptionsInputDlg::OptionsInputDlg(GameConfig& conf)
	: CDialog(IDD_OPTIONS_INPUT), m_conf(conf)
{
}

BOOL OptionsInputDlg::OnInitDialog()
{
    InitToolTip();

    CheckDlgButton(IDC_DIRECT_INPUT_CHECK, m_conf.direct_input ? BST_CHECKED : BST_UNCHECKED);

    return TRUE;
}

void OptionsInputDlg::InitToolTip()
{
    m_tool_tip.Create(*this);
    m_tool_tip.AddTool(GetDlgItem(IDC_DIRECT_INPUT_CHECK), "Use DirectInput for mouse input handling");
}

void OptionsInputDlg::OnSave()
{
    m_conf.direct_input = (IsDlgButtonChecked(IDC_DIRECT_INPUT_CHECK) == BST_CHECKED);
}
