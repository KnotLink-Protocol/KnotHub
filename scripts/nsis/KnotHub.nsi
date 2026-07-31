; KnotHub Combined Installer
; User-level install, no admin required.

!define PRODUCT_NAME        "KnotHub"
!define PRODUCT_VERSION      "0.2.1.0"
!define PRODUCT_PUBLISHER    "KnotLink"
!define CORE_EXE             "KnotHubCore.exe"
!define DASH_EXE             "knot-hub-dash.exe"
!define UNINST_KEY           "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define RUN_KEY              "Software\Microsoft\Windows\CurrentVersion\Run"

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "nsDialogs.nsh"

Var KLS_FOUND
Var KLS_VER

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
Page custom KlsCheckPage KlsCheckLeave
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "SimpChinese"

VIProductVersion                 "${PRODUCT_VERSION}"
VIAddVersionKey "ProductName"    "${PRODUCT_NAME}"
VIAddVersionKey "CompanyName"    "${PRODUCT_PUBLISHER}"
VIAddVersionKey "FileVersion"    "${PRODUCT_VERSION}"
VIAddVersionKey "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey "FileDescription" "KnotHub - KnotLink Service Hub & Dashboard"

RequestExecutionLevel user

Name    "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "..\bin\KnotHub-${PRODUCT_VERSION}-Setup.exe"
InstallDir "$LOCALAPPDATA\Programs\KnotHub"
ShowInstDetails   show
ShowUnInstDetails show

Section "Install"
    SetShellVarContext current
    SetOutPath "$INSTDIR"
    SetOverwrite ifnewer

    File /r "..\staging\*"

    CreateDirectory "$INSTDIR\Plugins"
    CreateDirectory "$INSTDIR\Recipes"

    CreateDirectory "$SMPROGRAMS\KnotHub"
    CreateShortCut "$SMPROGRAMS\KnotHub\KnotHub Dashboard.lnk" \
        "$INSTDIR\${DASH_EXE}"
    CreateShortCut "$SMPROGRAMS\KnotHub\Uninstall.lnk" \
        "$INSTDIR\uninst.exe"

    CreateShortCut "$DESKTOP\KnotHub.lnk" "$INSTDIR\${DASH_EXE}"

    WriteRegStr HKCU "${RUN_KEY}" "${PRODUCT_NAME}" \
        '"$INSTDIR\${CORE_EXE}"'

    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegStr   HKCU "${UNINST_KEY}" "DisplayName"     "${PRODUCT_NAME}"
    WriteRegStr   HKCU "${UNINST_KEY}" "UninstallString"  "$INSTDIR\uninst.exe"
    WriteRegStr   HKCU "${UNINST_KEY}" "DisplayIcon"      "$INSTDIR\${DASH_EXE}"
    WriteRegStr   HKCU "${UNINST_KEY}" "DisplayVersion"   "${PRODUCT_VERSION}"
    WriteRegStr   HKCU "${UNINST_KEY}" "Publisher"        "${PRODUCT_PUBLISHER}"
    WriteRegDWORD HKCU "${UNINST_KEY}" "EstimatedSize"     $0
    WriteRegDWORD HKCU "${UNINST_KEY}" "NoModify"          1
    WriteRegDWORD HKCU "${UNINST_KEY}" "NoRepair"          1

    WriteUninstaller "$INSTDIR\uninst.exe"

    ; Auto-start Core after install
    Exec '"$INSTDIR\${CORE_EXE}"'
SectionEnd

Section "Uninstall"
    SetShellVarContext current
    ExecWait 'taskkill /F /IM ${CORE_EXE}'
    ExecWait 'taskkill /F /IM ${DASH_EXE}'

    DeleteRegValue HKCU "${RUN_KEY}" "${PRODUCT_NAME}"
    DeleteRegKey   HKCU "${UNINST_KEY}"

    Delete "$SMPROGRAMS\KnotHub\KnotHub Dashboard.lnk"
    Delete "$SMPROGRAMS\KnotHub\Uninstall.lnk"
    RMDir  "$SMPROGRAMS\KnotHub"
    Delete "$DESKTOP\KnotHub.lnk"

    Delete "$INSTDIR\${CORE_EXE}"
    Delete "$INSTDIR\${DASH_EXE}"
    Delete "$INSTDIR\uninst.exe"
    RMDir /r "$INSTDIR\Plugins"
    RMDir /r "$INSTDIR\Recipes"
    RMDir "$INSTDIR"

    SetAutoClose true
SectionEnd

Var UNINST_PROG
Var OLD_VER
Var OLD_PATH

!define KLS_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\KnotLinkService"

; ── 自定义页面：检测 KnotLinkService ──────────────────────────
Function KlsCheckPage
    !insertmacro MUI_HEADER_TEXT "环境检测" "检查必要组件"

    ClearErrors
    ReadRegStr $KLS_VER HKLM "${KLS_UNINST_KEY}" "DisplayVersion"
    IfErrors 0 kls_yes
    StrCpy $KLS_FOUND "0"
    Goto show_page
kls_yes:
    StrCpy $KLS_FOUND "1"

show_page:
    nsDialogs::Create 1018
    Pop $0
    ${If} $KLS_FOUND == "1"
        ${NSD_CreateLabel} 0 20 100% 24 "✅ 已安装 KnotLinkService（版本 $KLS_VER）"
        Pop $0
        ${NSD_CreateLabel} 0 60 100% 40 "KnotLink 通信总线服务已就绪，KnotHub 可以正常使用。$\n点击「下一步」继续安装。"
        Pop $0
    ${Else}
        ${NSD_CreateLabel} 0 20 100% 24 "⚠ 未检测到 KnotLinkService"
        Pop $0
        ${NSD_CreateLabel} 0 60 100% 60 "KnotLinkService 是 KnotLink 通信总线，KnotHub 的所有功能都依赖它。$\n$\n点击「下一步」表示你已知晓风险，仍要继续安装。"
        Pop $0
        ${NSD_CreateLink} 0 130 100% 16 "🔗 下载 KnotLinkService（GitHub Releases）"
        Pop $0
        ${NSD_OnClick} $0 OpenKLSLink
    ${EndIf}
    nsDialogs::Show
FunctionEnd

Function OpenKLSLink
    ExecShell "open" "https://github.com/KnotLink-Protocol/KnotLinkService/releases"
FunctionEnd

Function KlsCheckLeave
    ${If} $KLS_FOUND == "0"
        MessageBox MB_YESNO|MB_ICONEXCLAMATION \
            "确定要在没有 KnotLinkService 的情况下继续安装吗？$\n$\nKnotHub 将无法正常工作。" \
            /SD IDNO IDYES +2
        Abort
    ${EndIf}
FunctionEnd

; ── .onInit：检测已有 KnotHub ────────────────────────────────
Function .onInit
    ClearErrors
    ReadRegStr $UNINST_PROG HKCU "${UNINST_KEY}" "UninstallString"
    IfErrors done

    ReadRegStr $OLD_VER HKCU "${UNINST_KEY}" "DisplayVersion"
    MessageBox MB_YESNOCANCEL|MB_ICONQUESTION \
        "已检测到 ${PRODUCT_NAME} $OLD_VER。$\n$\n是否卸载已有版本？" \
        /SD IDYES IDYES uninstall IDNO done
    Abort

uninstall:
    StrCpy $OLD_PATH $UNINST_PROG -10
    ExecWait '"$UNINST_PROG" /S _?=$OLD_PATH' $0
    DetailPrint "uninst.exe returned $0"
    Delete "$UNINST_PROG"
    RMDir $OLD_PATH
done:
FunctionEnd
