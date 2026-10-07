#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <map>
#include <cwctype>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

namespace fs = std::filesystem;

struct Line { std::wstring raw, section, key, value; bool kv=false; };
static std::vector<Line> lines;
static std::wstring currentFile;
static HWND hTech, hValues, hStatus;
static HBRUSH hBg, hPanel, hEdit, hAccent;
static HFONT hFont, hTitle;
static COLORREF PURPLE=RGB(170,90,255), BG=RGB(18,14,24), PANEL=RGB(28,22,38), TEXT=RGB(238,232,245), MUTED=RGB(165,150,180);

static std::wstring trim(std::wstring s){
    while(!s.empty() && iswspace(s.front())) s.erase(s.begin());
    while(!s.empty() && iswspace(s.back())) s.pop_back();
    return s;
}
static std::vector<std::wstring> splitComma(const std::wstring& s){
    std::vector<std::wstring> out; std::wstringstream ss(s); std::wstring x;
    while(std::getline(ss,x,L',')) { x=trim(x); if(!x.empty()) out.push_back(x); }
    return out;
}
static std::wstring joinComma(const std::vector<std::wstring>& v){
    std::wstring r; for(size_t i=0;i<v.size();++i){ if(i) r+=L","; r+=v[i]; } return r;
}
static bool isNumber(const std::wstring& s){
    if(s.empty()) return false; wchar_t* e=nullptr; wcstod(s.c_str(), &e); return e && *e==0;
}
static void SetStatus(const std::wstring& s){ SetWindowTextW(hStatus,s.c_str()); }

static bool LoadPreset(const std::wstring& path){
    std::wifstream f(path); f.imbue(std::locale(""));
    if(!f) return false;
    lines.clear(); std::wstring sec, s;
    while(std::getline(f,s)){
        Line l; l.raw=s; l.section=sec;
        auto p=s.find(L'=');
        if(!s.empty() && s.front()==L'[' && s.back()==L']') { sec=s.substr(1,s.size()-2); l.section=sec; }
        else if(p!=std::wstring::npos && !s.empty() && s.front()!=L';' && s.front()!=L'#'){
            l.kv=true; l.key=trim(s.substr(0,p)); l.value=trim(s.substr(p+1)); l.section=sec;
        }
        lines.push_back(l);
    }
    currentFile=path; return true;
}
static int FindKey(const std::wstring& section,const std::wstring& key){
    for(int i=0;i<(int)lines.size();++i) if(lines[i].kv && _wcsicmp(lines[i].section.c_str(),section.c_str())==0 && _wcsicmp(lines[i].key.c_str(),key.c_str())==0) return i;
    return -1;
}
static std::wstring GetKey(const std::wstring& section,const std::wstring& key){ int i=FindKey(section,key); return i>=0?lines[i].value:L""; }
static void SetKey(const std::wstring& section,const std::wstring& key,const std::wstring& value){
    int i=FindKey(section,key); if(i>=0){ lines[i].value=value; lines[i].raw=key+L"="+value; return; }
    // Add to end under a section header if it exists, otherwise create one.
    lines.push_back({L"["+section+L"]",section,L"",L"",false});
    lines.push_back({key+L"="+value,section,key,value,true});
}
static void RefreshTechniques(){
    SendMessageW(hTech,LB_RESETCONTENT,0,0);
    std::wstring t=GetKey(L"GLOBAL",L"Techniques"); if(t.empty()) t=GetKey(L"GENERAL",L"Techniques");
    for(auto &x:splitComma(t)) SendMessageW(hTech,LB_ADDSTRING,0,(LPARAM)x.c_str());
}
static void RefreshValues(){
    SendMessageW(hValues,LB_RESETCONTENT,0,0);
    const wchar_t* secs[]={L"GLOBAL",L"GENERAL"};
    for(auto sec:secs) for(auto &l:lines) if(l.kv && _wcsicmp(l.section.c_str(),sec)==0 && isNumber(l.value)){
        std::wstring row=l.key+L" = "+l.value; SendMessageW(hValues,LB_ADDSTRING,0,(LPARAM)row.c_str());
    }
}
static void SavePreset(){
    if(currentFile.empty()) return;
    try { fs::copy_file(currentFile,currentFile+L".bak",fs::copy_options::overwrite_existing); } catch(...){}
    std::wofstream f(currentFile); f.imbue(std::locale(""));
    for(auto &l:lines) f<<l.raw<<L"\n";
    SetStatus(L"Saved preset • backup created as .bak");
}
static void ToggleSelectedTechnique(){
    int n=(int)SendMessageW(hTech,LB_GETSELCOUNT,0,0); if(n<=0) return;
    std::vector<int> idx(n); SendMessageW(hTech,LB_GETSELITEMS,n,(LPARAM)idx.data());
    std::wstring key=GetKey(L"GLOBAL",L"Techniques"); std::vector<std::wstring> v=splitComma(key);
    for(int k=n-1;k>=0;--k){ wchar_t buf[256]; SendMessageW(hTech,LB_GETTEXT,idx[k],(LPARAM)buf); std::wstring name=buf;
        auto it=std::find(v.begin(),v.end(),name); if(it!=v.end()) v.erase(it); else v.push_back(name);
    }
    SetKey(L"GLOBAL",L"Techniques",joinComma(v)); RefreshTechniques(); RefreshValues(); SetStatus(L"Technique list updated • save to apply");
}
static void EditSelectedValue(){
    int s=(int)SendMessageW(hValues,LB_GETCURSEL,0,0); if(s==LB_ERR) return;
    wchar_t buf[512]; SendMessageW(hValues,LB_GETTEXT,s,(LPARAM)buf); std::wstring row=buf;
    auto p=row.find(L" = "); if(p==std::wstring::npos) return; std::wstring key=row.substr(0,p), val=row.substr(p+3);
    // Find matching numeric key in GLOBAL first, then GENERAL.
    int i=FindKey(L"GLOBAL",key); if(i<0) i=FindKey(L"GENERAL",key); if(i<0) return;
    wchar_t msg[512]; swprintf(msg,512,L"Enter a new value for %s:\nCurrent: %s",key.c_str(),lines[i].value.c_str());
    // Use a tiny prompt window implemented by MessageBox is intentionally avoided; edit via clipboard-style simple input dialog below.
    HWND dlg=CreateWindowExW(WS_EX_DLGMODALFRAME,L"STATIC",L"Edit Value",WS_POPUP|WS_CAPTION|WS_SYSMENU,0,0,380,140,GetParent(hValues),nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!dlg) return;
    HWND e=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",lines[i].value.c_str(),WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,20,35,330,25,dlg,(HMENU)1,GetModuleHandleW(nullptr),nullptr);
    HWND b=CreateWindowW(L"BUTTON",L"OK",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,250,75,100,28,dlg,(HMENU)2,GetModuleHandleW(nullptr),nullptr);
    HWND c=CreateWindowW(L"BUTTON",L"Cancel",WS_CHILD|WS_VISIBLE,140,75,100,28,dlg,(HMENU)3,GetModuleHandleW(nullptr),nullptr);
    ShowWindow(dlg,SW_SHOW); SetForegroundWindow(dlg); MSG m; bool done=false, ok=false;
    while(!done && GetMessageW(&m,nullptr,0,0)){ if(m.message==WM_COMMAND && (LOWORD(m.wParam)==2||LOWORD(m.wParam)==3) && (HWND)m.hwnd==dlg){ ok=LOWORD(m.wParam)==2; done=true; } TranslateMessage(&m); DispatchMessageW(&m); if(m.message==WM_CLOSE) done=true; }
    if(ok){ wchar_t nv[256]; GetWindowTextW(e,nv,256); if(isNumber(nv)){ lines[i].value=nv; lines[i].raw=key+L"="+nv; SetStatus(L"Value updated • save to apply"); } }
    DestroyWindow(dlg); RefreshValues();
}
static void BrowsePreset(){
    OPENFILENAMEW ofn{}; wchar_t file[MAX_PATH]=L""; ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=GetForegroundWindow(); ofn.lpstrFilter=L"ReShade presets (*.ini)\0*.ini\0All files\0*.*\0"; ofn.lpstrFile=file; ofn.nMaxFile=MAX_PATH; ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
    if(GetOpenFileNameW(&ofn)){ if(LoadPreset(file)){ RefreshTechniques(); RefreshValues(); SetStatus(L"Loaded: "+std::wstring(file)); } }
}
static void BrowseFolder(){
    if(currentFile.empty()){ BrowsePreset(); return; }
    ShellExecuteW(nullptr,L"open",L"explorer.exe",(L"/select,\""+currentFile+L"\"").c_str(),nullptr,SW_SHOWNORMAL);
}
static LRESULT CALLBACK WndProc(HWND w,UINT m,WPARAM wp,LPARAM lp){
    switch(m){
    case WM_CREATE:{
        hBg=CreateSolidBrush(BG); hPanel=CreateSolidBrush(PANEL); hEdit=CreateSolidBrush(RGB(35,28,48)); hAccent=CreateSolidBrush(PURPLE);
        hFont=CreateFontW(17,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        hTitle=CreateFontW(28,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        CreateWindowW(L"STATIC",L"CLEAN RESHADE",WS_CHILD|WS_VISIBLE,24,18,400,38,w,nullptr,nullptr,nullptr);
        CreateWindowW(L"STATIC",L"PURPLE EDITION  •  PRESET CONTROL",WS_CHILD|WS_VISIBLE,27,53,400,22,w,nullptr,nullptr,nullptr);
        auto btn=[&](LPCWSTR t,int x,int id){return CreateWindowW(L"BUTTON",t,WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,x,86,145,34,w,(HMENU)id,nullptr,nullptr);};
        btn(L"OPEN PRESET",24,101); btn(L"SAVE PRESET",180,102); btn(L"PRESET FOLDER",336,103); btn(L"TOGGLE SELECTED",492,104);
        CreateWindowW(L"STATIC",L"TECHNIQUES",WS_CHILD|WS_VISIBLE,24,137,300,25,w,nullptr,nullptr,nullptr);
        hTech=CreateWindowExW(WS_EX_CLIENTEDGE,L"LISTBOX",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_EXTENDEDSEL|LBS_NOINTEGRALHEIGHT,24,165,360,315,w,(HMENU)201,nullptr,nullptr);
        CreateWindowW(L"STATIC",L"NUMERIC VALUES",WS_CHILD|WS_VISIBLE,410,137,300,25,w,nullptr,nullptr,nullptr);
        hValues=CreateWindowExW(WS_EX_CLIENTEDGE,L"LISTBOX",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOINTEGRALHEIGHT,410,165,360,315,w,(HMENU)202,nullptr,nullptr);
        CreateWindowW(L"BUTTON",L"EDIT SELECTED VALUE",WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,410,490,180,34,w,(HMENU)105,nullptr,nullptr);
        hStatus=CreateWindowW(L"STATIC",L"Open a ReShade preset to begin.",WS_CHILD|WS_VISIBLE,24,535,746,30,w,nullptr,nullptr,nullptr);
        SendMessageW(hTech,WM_SETFONT,(WPARAM)hFont,TRUE); SendMessageW(hValues,WM_SETFONT,(WPARAM)hFont,TRUE); SendMessageW(hStatus,WM_SETFONT,(WPARAM)hFont,TRUE); return 0; }
    case WM_CTLCOLORSTATIC:{ HDC dc=(HDC)wp; SetTextColor(dc,TEXT); SetBkColor(dc,BG); return (LRESULT)hBg; }
    case WM_CTLCOLOREDIT: { HDC dc=(HDC)wp; SetTextColor(dc,TEXT); SetBkColor(dc,RGB(35,28,48)); return (LRESULT)hEdit; }
    case WM_DRAWITEM:{ auto d=(DRAWITEMSTRUCT*)lp; HBRUSH b=CreateSolidBrush(PURPLE); FillRect(d->hDC,&d->rcItem,b); DeleteObject(b); SetTextColor(d->hDC,RGB(255,255,255)); SetBkMode(d->hDC,TRANSPARENT); wchar_t txt[128]; GetWindowTextW(d->hwndItem,txt,128); DrawTextW(d->hDC,txt,-1,&d->rcItem,DT_CENTER|DT_VCENTER|DT_SINGLELINE); return TRUE; }
    case WM_COMMAND:
        switch(LOWORD(wp)){ case 101: BrowsePreset(); break; case 102: SavePreset(); break; case 103: BrowseFolder(); break; case 104: ToggleSelectedTechnique(); break; case 105: EditSelectedValue(); break; } return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(w,m,wp,lp);
}
int WINAPI wWinMain(HINSTANCE h,HINSTANCE, PWSTR, int n){
    INITCOMMONCONTROLSEX ic{sizeof(ic),ICC_STANDARD_CLASSES}; InitCommonControlsEx(&ic);
    WNDCLASSW wc{}; wc.lpfnWndProc=WndProc; wc.hInstance=h; wc.lpszClassName=L"CleanReshadePurple"; wc.hCursor=LoadCursor(nullptr,IDC_ARROW); wc.hbrBackground=CreateSolidBrush(BG); RegisterClassW(&wc);
    HWND w=CreateWindowExW(0,wc.lpszClassName,L"CLEAN RESHADE — Purple",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,100,100,810,620,nullptr,nullptr,h,nullptr);
    ShowWindow(w,n); UpdateWindow(w);
    MSG msg{}; while(GetMessageW(&msg,nullptr,0,0)){ TranslateMessage(&msg); DispatchMessageW(&msg); } return 0;
}
