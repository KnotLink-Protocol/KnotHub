; KnotHub Combined Installer
; User-level install, no admin required.

!define PRODUCT_NAME        "KnotHub"
!define PRODUCT_VERSION      "1.0.0.0"
!define PRODUCT_PUBLISHER    "KnotLink"
!define CORE_EXE             "KnotHubCore.exe"
!define DASH_EXE             "knot-hub-dash.exe"
!define UNINST_KEY           "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define RUN_KEY              "Software\Microsoft\Windows\CurrentVersion\Run"

!include "MUI2.nsh"
!include "FileFunc.nsh"

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
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

    File "..\staging\${CORE_EXE}"
    File "..\staging\${DASH_EXE}"

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

Function .onInit
    ClearErrors
    ReadRegStr $UNINST_PROG HKCU "${UNINST_KEY}" "UninstallString"
    IfErrors done

    ReadRegStr $OLD_VER HKCU "${UNINST_KEY}" "DisplayVersion"
    MessageBox MB_YESNOCANCEL|MB_ICONQUESTION \
        "Detected ${PRODUCT_NAME} $OLD_VER is already installed.$\n$\nUninstall the existing version?" \
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
