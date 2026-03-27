// NetworkDlg.cpp - 网络配置对话框实现

#include "stdafx.h"
#include "NetworkDlg.h"
#include "afxdialogex.h"
#include "2015Remote.h"

CNetworkDlg::CNetworkDlg(CWnd* pParent /*=nullptr*/)
    : CDialogLangEx(IDD_DIALOG_NETWORK, pParent)
{
}

CNetworkDlg::~CNetworkDlg()
{
}

void CNetworkDlg::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_WHITELIST, m_ListWhitelist);
    DDX_Control(pDX, IDC_LIST_BLACKLIST, m_ListBlacklist);
    DDX_Control(pDX, IDC_EDIT_NETWORK_IP, m_EditIP);
}

BEGIN_MESSAGE_MAP(CNetworkDlg, CDialogLangEx)
    ON_BN_CLICKED(IDC_BTN_ADD_WHITELIST, &CNetworkDlg::OnBnClickedAddWhitelist)
    ON_BN_CLICKED(IDC_BTN_DEL_WHITELIST, &CNetworkDlg::OnBnClickedDelWhitelist)
    ON_BN_CLICKED(IDC_BTN_ADD_BLACKLIST, &CNetworkDlg::OnBnClickedAddBlacklist)
    ON_BN_CLICKED(IDC_BTN_DEL_BLACKLIST, &CNetworkDlg::OnBnClickedDelBlacklist)
    ON_BN_CLICKED(IDOK, &CNetworkDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CNetworkDlg::OnInitDialog()
{
    __super::OnInitDialog();

    // 多语言翻译
    SetWindowText(_TR("网络配置"));
    SetDlgItemText(IDC_STATIC_WHITELIST, _TR("白名单 (不限流):"));
    SetDlgItemText(IDC_STATIC_BLACKLIST, _TR("黑名单 (拒绝连接):"));
    SetDlgItemText(IDC_STATIC_IP_INPUT, _TR("IP 地址:"));
    SetDlgItemText(IDC_BTN_ADD_WHITELIST, _TR("添加 >>"));
    SetDlgItemText(IDC_BTN_DEL_WHITELIST, _TR("删除"));
    SetDlgItemText(IDC_BTN_ADD_BLACKLIST, _TR("添加 >>"));
    SetDlgItemText(IDC_BTN_DEL_BLACKLIST, _TR("删除"));
    SetDlgItemText(IDC_STATIC_NETWORK_HINT, _TR("说明:"));
    SetDlgItemText(IDC_STATIC_NETWORK_HINT1, _TR("- 白名单IP: 不受限流"));
    SetDlgItemText(IDC_STATIC_NETWORK_HINT2, _TR("- 黑名单IP: 拒绝请求"));
    SetDlgItemText(IDC_STATIC_NETWORK_HINT3, _TR("- 127.0.0.1 自动白名单"));
    SetDlgItemText(IDC_STATIC_NETWORK_HINT4, _TR("- 配置即时生效并保存"));
    SetDlgItemText(IDOK, _TR("确定"));
    SetDlgItemText(IDCANCEL, _TR("取消"));

    // 加载现有列表
    LoadLists();

    return TRUE;
}

void CNetworkDlg::LoadLists()
{
    // 加载白名单
    m_ListWhitelist.ResetContent();
    auto whiteIPs = IPWhitelist::getInstance().GetAll();
    for (const auto& ip : whiteIPs) {
        m_ListWhitelist.AddString(CString(ip.c_str()));
    }

    // 加载黑名单
    m_ListBlacklist.ResetContent();
    auto blackIPs = IPBlacklist::getInstance().GetAll();
    for (const auto& ip : blackIPs) {
        m_ListBlacklist.AddString(CString(ip.c_str()));
    }
}

void CNetworkDlg::SaveLists()
{
    // 保存白名单
    std::string whitelistStr = IPWhitelist::getInstance().Export();
    THIS_CFG.SetStr("settings", "IPWhitelist", whitelistStr.c_str());

    // 保存黑名单
    std::string blacklistStr = IPBlacklist::getInstance().Export();
    THIS_CFG.SetStr("settings", "IPBlacklist", blacklistStr.c_str());
}

bool CNetworkDlg::IsValidIP(const CString& ip)
{
    if (ip.IsEmpty()) return false;

    // 简单的 IP 格式检查: xxx.xxx.xxx.xxx
    int partCount = 0;
    int partValue = 0;
    bool hasDigit = false;

    for (int i = 0; i <= ip.GetLength(); i++) {
        TCHAR c = (i < ip.GetLength()) ? ip.GetAt(i) : '\0';

        if (c >= '0' && c <= '9') {
            partValue = partValue * 10 + (c - '0');
            hasDigit = true;
            if (partValue > 255) return false;
        }
        else if (c == '.' || c == '\0') {
            if (!hasDigit) return false;  // 空段
            partCount++;
            partValue = 0;
            hasDigit = false;
            if (c == '\0') break;
        }
        else {
            return false;  // 非法字符
        }
    }

    return (partCount == 4);
}

CString CNetworkDlg::GetInputIP()
{
    CString ip;
    m_EditIP.GetWindowText(ip);
    ip.Trim();
    return ip;
}

void CNetworkDlg::OnBnClickedAddWhitelist()
{
    CString ip = GetInputIP();

    if (!IsValidIP(ip)) {
        MessageBox(_TR("请输入有效的 IP 地址"), _TR("错误"), MB_OK | MB_ICONERROR);
        return;
    }

    CStringA ipA(ip);
    std::string ipStr(ipA.GetString());

    // 检查是否已在白名单中
    if (IPWhitelist::getInstance().IsWhitelisted(ipStr)) {
        MessageBox(_TR("该 IP 已在白名单中"), _TR("提示"), MB_OK | MB_ICONINFORMATION);
        return;
    }

    // 如果在黑名单中，先移除
    if (IPBlacklist::getInstance().IsBlacklisted(ipStr)) {
        IPBlacklist::getInstance().RemoveIP(ipStr);
        // 从黑名单列表中移除
        for (int i = 0; i < m_ListBlacklist.GetCount(); i++) {
            CString item;
            m_ListBlacklist.GetText(i, item);
            if (item == ip) {
                m_ListBlacklist.DeleteString(i);
                break;
            }
        }
    }

    // 添加到白名单
    IPWhitelist::getInstance().AddIP(ipStr);
    m_ListWhitelist.AddString(ip);
    m_EditIP.SetWindowText(_T(""));
}

void CNetworkDlg::OnBnClickedDelWhitelist()
{
    int sel = m_ListWhitelist.GetCurSel();
    if (sel == LB_ERR) {
        MessageBox(_TR("请先选择要删除的 IP"), _TR("提示"), MB_OK | MB_ICONINFORMATION);
        return;
    }

    CString ip;
    m_ListWhitelist.GetText(sel, ip);
    CStringA ipA(ip);
    std::string ipStr(ipA.GetString());

    IPWhitelist::getInstance().RemoveIP(ipStr);
    m_ListWhitelist.DeleteString(sel);
}

void CNetworkDlg::OnBnClickedAddBlacklist()
{
    CString ip = GetInputIP();

    if (!IsValidIP(ip)) {
        MessageBox(_TR("请输入有效的 IP 地址"), _TR("错误"), MB_OK | MB_ICONERROR);
        return;
    }

    CStringA ipA(ip);
    std::string ipStr(ipA.GetString());

    // 检查是否是本地地址
    if (ipStr == "127.0.0.1" || ipStr == "::1") {
        MessageBox(_TR("本地地址不能加入黑名单"), _TR("错误"), MB_OK | MB_ICONERROR);
        return;
    }

    // 检查是否已在黑名单中
    if (IPBlacklist::getInstance().IsBlacklisted(ipStr)) {
        MessageBox(_TR("该 IP 已在黑名单中"), _TR("提示"), MB_OK | MB_ICONINFORMATION);
        return;
    }

    // 如果在白名单中，先移除
    if (IPWhitelist::getInstance().IsWhitelisted(ipStr)) {
        IPWhitelist::getInstance().RemoveIP(ipStr);
        // 从白名单列表中移除
        for (int i = 0; i < m_ListWhitelist.GetCount(); i++) {
            CString item;
            m_ListWhitelist.GetText(i, item);
            if (item == ip) {
                m_ListWhitelist.DeleteString(i);
                break;
            }
        }
    }

    // 添加到黑名单
    IPBlacklist::getInstance().AddIP(ipStr);
    m_ListBlacklist.AddString(ip);
    m_EditIP.SetWindowText(_T(""));
}

void CNetworkDlg::OnBnClickedDelBlacklist()
{
    int sel = m_ListBlacklist.GetCurSel();
    if (sel == LB_ERR) {
        MessageBox(_TR("请先选择要删除的 IP"), _TR("提示"), MB_OK | MB_ICONINFORMATION);
        return;
    }

    CString ip;
    m_ListBlacklist.GetText(sel, ip);
    CStringA ipA(ip);
    std::string ipStr(ipA.GetString());

    IPBlacklist::getInstance().RemoveIP(ipStr);
    m_ListBlacklist.DeleteString(sel);
}

void CNetworkDlg::OnBnClickedOk()
{
    // 保存配置
    SaveLists();

    __super::OnOK();
}
