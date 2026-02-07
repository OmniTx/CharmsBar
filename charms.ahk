#Requires AutoHotkey v2.0
; ==============================================================================
; Script: Ultimate Charms Bar (Gold Master - Dark Glass Edition)
; Features: CPU Optimized | RGB Wave | Superscript Clock | No Blur
; ==============================================================================

; --- Configuration Defaults ---
Global BarWidth := 160
Global HoverDelay := 400
Global IniFile := A_ScriptDir "\CharmsSettings.ini"

; --- Load Settings ---
Global SearchProvider := IniRead(IniFile, "Settings", "SearchProvider", "Windows")
Global EverythingPath := IniRead(IniFile, "Paths", "EverythingPath", "C:\Program Files\Everything 1.5a\Everything.exe")

; Theme (Defaulting to Dark Glass)
Global CurrentTheme   := IniRead(IniFile, "Theme", "Mode", "Dark") 
Global CustomColor    := IniRead(IniFile, "Theme", "CustomColor", "000000")

; Apps
Global AppA_Name := IniRead(IniFile, "AppA", "Name", "Notepad")
Global AppA_Path := IniRead(IniFile, "AppA", "Path", "notepad.exe")
Global AppB_Name := IniRead(IniFile, "AppB", "Name", "File Explorer")
Global AppB_Path := IniRead(IniFile, "AppB", "Path", "explorer.exe")
Global AppC_Name := IniRead(IniFile, "AppC", "Name", "Cmd")
Global AppC_Path := IniRead(IniFile, "AppC", "Path", "cmd.exe")

; --- State Variables ---
Global CharmsBar := ""
Global ConfigGui := ""
Global OpenedByHover := false
Global CornerHoverStart := 0
Global TimeCtl := "", AmPmCtl := "", DateCtl := ""
Global BtnA := "", BtnB := "", BtnC := "" 
Global LastToolTip := "" 
Global Hue := 0 

; Start Watchers (Only essential ones run constantly)
SetTimer(HotCornerWatch, 100)
OnMessage(0x0200, CheckTooltips)

; Hotkey
#c::ToggleCharmsBar(false)

; ==============================================================================
; CORE LOGIC
; ==============================================================================

HotCornerWatch() {
    Global OpenedByHover, CharmsBar, CornerHoverStart
    CoordMode "Mouse", "Screen"
    MouseGetPos &MX, &MY
    MonitorGet(MonitorGetPrimary(), &L, &T, &R, &B)
    
    InCorner := (MX >= (R - 10) && MY <= (T + 10))

    if (InCorner && !CharmsBar) {
        if (CornerHoverStart == 0)
            CornerHoverStart := A_TickCount
        else if (A_TickCount - CornerHoverStart > HoverDelay) {
            OpenedByHover := true
            ToggleCharmsBar(true)
            CornerHoverStart := 0 
        }
    } else {
        CornerHoverStart := 0
    }

    if (CharmsBar && OpenedByHover) {
        if (MX < (R - BarWidth)) {
            HideCharms()
            OpenedByHover := false
        }
    }
}

ToggleCharmsBar(fromHover := false) {
    Global CharmsBar, OpenedByHover
    
    if (CharmsBar && WinExist("ahk_id " CharmsBar.Hwnd)) {
        if (!fromHover && OpenedByHover) {
            OpenedByHover := false 
            CreateCharmsBar() 
            return
        }
        HideCharms()
        return
    }

    OpenedByHover := fromHover
    CreateCharmsBar()

    MonitorGet(MonitorGetPrimary(), &L, &T, &R, &B)
    BarH := B - T
    BarX := R - BarWidth
    BarY := T

    ; Show window
    CharmsBar.Show("x" BarX " y" BarY " w" BarWidth " h" BarH " NoActivate Hide")
    
    ; Animation (Slide Left)
    DllCall("AnimateWindow", "Ptr", CharmsBar.Hwnd, "Int", 150, "Int", 0x40000 | 0x00002)
    
    try {
        WinActivate("ahk_id " CharmsBar.Hwnd)
    }
    
    ; START TIMERS (CPU Optimization: Only run these when visible)
    SetTimer(CheckFocus, 100)
    SetTimer(UpdateClock, 1000)
    SetTimer(RainbowCycle, 50) 
}

CheckFocus() {
    Global CharmsBar, ConfigGui
    if (ConfigGui && WinActive("ahk_id " ConfigGui.Hwnd))
        return

    if (CharmsBar && !WinActive("ahk_id " CharmsBar.Hwnd)) {
        HideCharms()
        OpenedByHover := false
    }
}

HideCharms() {
    Global CharmsBar
    
    ; STOP TIMERS (Save CPU)
    SetTimer(CheckFocus, 0)
    SetTimer(UpdateClock, 0)
    SetTimer(RainbowCycle, 0)
    
    ToolTip() 

    if (!CharmsBar || !WinExist("ahk_id " CharmsBar.Hwnd))
        return

    ; Animation (Slide Right)
    DllCall("AnimateWindow", "Ptr", CharmsBar.Hwnd, "Int", 150, "Int", 0x10000 | 0x40000 | 0x00001)
    
    if (CharmsBar)
        CharmsBar.Destroy()
    CharmsBar := ""
}

; ==============================================================================
; RGB & Clock Logic
; ==============================================================================

RainbowCycle() {
    Global Hue, BtnA, BtnB, BtnC, CharmsBar
    
    if (!CharmsBar || !BtnA)
        return

    Hue := Mod(Hue + 4, 360) 
    
    ; Wave Effect (Phase Shift)
    ColorA := HSVtoRGB(Hue, 1, 1)
    ColorB := HSVtoRGB(Mod(Hue + 60, 360), 1, 1) 
    ColorC := HSVtoRGB(Mod(Hue + 120, 360), 1, 1)

    try {
        BtnA.Opt("c" ColorA)
        BtnB.Opt("c" ColorB)
        BtnC.Opt("c" ColorC)
    }
}

HSVtoRGB(h, s, v) {
    c := v * s
    x := c * (1 - Abs(Mod(h / 60, 2) - 1))
    m := v - c
    
    if (h < 60)
        r:=c, g:=x, b:=0
    else if (h < 120)
        r:=x, g:=c, b:=0
    else if (h < 180)
        r:=0, g:=c, b:=x
    else if (h < 240)
        r:=0, g:=x, b:=c
    else if (h < 300)
        r:=x, g:=0, b:=c
    else
        r:=c, g:=0, b:=x
        
    R := Format("{:02X}", (r + m) * 255)
    G := Format("{:02X}", (g + m) * 255)
    B := Format("{:02X}", (b + m) * 255)
    return R G B
}

UpdateClock() {
    Global TimeCtl, AmPmCtl, DateCtl
    if (TimeCtl) {
        TimeCtl.Text := FormatTime(, "h:mm")
        AmPmCtl.Text := FormatTime(, "tt")
    }
}

; ==============================================================================
; GUI Creation
; ==============================================================================

CreateCharmsBar() {
    Global CharmsBar, TimeCtl, AmPmCtl, DateCtl, BtnA, BtnB, BtnC
    
    Colors := GetThemeColors()
    TC := Colors.Text
    IC := Colors.Icon
    BC := Colors.Bg

    CharmsBar := Gui("-Caption +AlwaysOnTop +ToolWindow +Owner +E0x08000000", "CharmsBar")
    CharmsBar.BackColor := BC
    CharmsBar.MarginX := 0
    CharmsBar.MarginY := 0
    
    ; Dark Glass Transparency (No Blur, just efficient alpha)
    WinSetTransparent(230, CharmsBar) 
    
    CharmsBar.OnEvent("Escape", (*) => HideCharms())

    ; --- Header ---
    CharmsBar.SetFont("s10 c" TC, "Segoe UI Semibold")
    CharmsBar.Add("Text", "x0 y10 w" BarWidth " Center BackgroundTrans", A_UserName)
    CharmsBar.SetFont("s9 cGray", "Segoe UI") 
    CharmsBar.Add("Text", "x0 y+2 w" BarWidth " Center BackgroundTrans", GetBatteryInfo())
    AddDivider(TC)

    ; --- Charms ---
    CharmsBar.SetFont("s24 c" IC, "Segoe MDL2 Assets")
    AddCharm(0xE721, "Search", (*) => (HideCharms(), ExecuteSearch()), "Search System/Files", TC)
    AddCharm(0xF0E3, "Clipboard", (*) => (HideCharms(), Send("#v")), "Clipboard History (Win+V)", TC)
    AddCharm(0xE80F, "Start", (*) => (HideCharms(), Send("{LWin}")), "Open Start Menu", TC)
    AddCharm(0xE772, "Devices", (*) => (HideCharms(), Send("#k")), "Connect Devices (Win+K)", TC)
    AddCharm(0xE713, "Settings", (*) => (HideCharms(), Send("#i")), "Windows Settings", TC)

    AddDivider(TC)

    ; --- Media ---
    CharmsBar.SetFont("s9 cGray", "Segoe UI")
    CharmsBar.Add("Text", "x0 y+10 w" BarWidth " Center BackgroundTrans", "MEDIA")
    CharmsBar.SetFont("s16 c" IC, "Segoe MDL2 Assets")
    BW := 40
    StartX := (BarWidth - (BW * 3)) / 2
    
    PrevBtn := CharmsBar.Add("Text", "x" StartX " y+5 w" BW " h30 Center +0x200 BackgroundTrans c" IC, Chr(0xE892))
    PrevBtn.OnEvent("Click", (*) => Send("{Media_Prev}"))
    PrevBtn.ToolTipText := "Previous Track"
    
    PlayBtn := CharmsBar.Add("Text", "x+0 w" BW " h30 Center +0x200 BackgroundTrans c" IC, Chr(0xE768))
    PlayBtn.OnEvent("Click", (*) => Send("{Media_Play_Pause}"))
    PlayBtn.ToolTipText := "Play / Pause"

    NextBtn := CharmsBar.Add("Text", "x+0 w" BW " h30 Center +0x200 BackgroundTrans c" IC, Chr(0xE893))
    NextBtn.OnEvent("Click", (*) => Send("{Media_Next}"))
    NextBtn.ToolTipText := "Next Track"

    CharmsBar.SetFont("s10 c" TC, "Segoe UI")
    VolSlider := CharmsBar.Add("Slider", "x15 y+5 w" (BarWidth-30) " h20 ToolTip NoTicks Background" BC, SoundGetVolume())
    VolSlider.OnEvent("Change", (GuiCtrl, *) => SoundSetVolume(GuiCtrl.Value))

    AddDivider(TC)

    ; --- Tools ---
    CharmsBar.SetFont("s9 cGray", "Segoe UI")
    CharmsBar.Add("Text", "x0 y+10 w" BarWidth " Center BackgroundTrans", "TOOLS")
    CharmsBar.SetFont("s14 c" IC, "Segoe MDL2 Assets")
    
    SnipBtn := CharmsBar.Add("Text", "x20 y+5 w40 h40 Center +0x200 BackgroundTrans c" IC, Chr(0xE70F))
    SnipBtn.OnEvent("Click", (*) => (HideCharms(), Send("#+s")))
    SnipBtn.ToolTipText := "Snipping Tool"

    CalcBtn := CharmsBar.Add("Text", "x+0 w40 h40 Center +0x200 BackgroundTrans c" IC, Chr(0xE8EF))
    CalcBtn.OnEvent("Click", (*) => (HideCharms(), Run("calc.exe")))
    CalcBtn.ToolTipText := "Calculator"

    TaskBtn := CharmsBar.Add("Text", "x+0 w40 h40 Center +0x200 BackgroundTrans c" IC, Chr(0xE9D9))
    TaskBtn.OnEvent("Click", (*) => (HideCharms(), Run("taskmgr.exe")))
    TaskBtn.ToolTipText := "Task Manager"

    AddDivider(TC)

    ; --- Apps (RGB Mode) ---
    CharmsBar.SetFont("s9 cGray", "Segoe UI")
    CharmsBar.Add("Text", "x0 y+10 w" BarWidth " Center BackgroundTrans", "APPS")
    CharmsBar.SetFont("s12 w700", "Segoe UI") 
    
    BtnA := CharmsBar.Add("Text", "x20 y+5 w40 h40 Center +0x200 BackgroundTrans", "A")
    BtnA.OnEvent("Click", (*) => (HideCharms(), RunApp(AppA_Path)))
    BtnA.ToolTipText := AppA_Name 

    BtnB := CharmsBar.Add("Text", "x+0 w40 h40 Center +0x200 BackgroundTrans", "B")
    BtnB.OnEvent("Click", (*) => (HideCharms(), RunApp(AppB_Path)))
    BtnB.ToolTipText := AppB_Name

    BtnC := CharmsBar.Add("Text", "x+0 w40 h40 Center +0x200 BackgroundTrans", "C")
    BtnC.OnEvent("Click", (*) => (HideCharms(), RunApp(AppC_Path)))
    BtnC.ToolTipText := AppC_Name

    ; --- Bottom Dock ---
    MonitorGet(MonitorGetPrimary(), &L, &T, &R, &B)
    BarH := B - T
    BottomY := BarH - 140 

    ; CLOCK (Superscript Layout)
    CharmsBar.SetFont("s26 c" TC, "Segoe UI Light")
    TimeCtl := CharmsBar.Add("Text", "x0 y" BottomY " w100 Right BackgroundTrans", FormatTime(, "h:mm"))
    
    CharmsBar.SetFont("s10 c" TC, "Segoe UI")
    AmPmCtl := CharmsBar.Add("Text", "x+5 yp+12 w50 Left BackgroundTrans", FormatTime(, "tt"))

    CharmsBar.SetFont("s10 cGray", "Segoe UI")
    CharmsBar.Add("Text", "x0 y+10 w" BarWidth " Center BackgroundTrans", FormatTime(, "M/d/yyyy"))

    ; Power Button
    CharmsBar.SetFont("s20 c" IC, "Segoe MDL2 Assets")
    PowerBtn := CharmsBar.Add("Text", "x0 y+10 w" BarWidth " h40 Center +0x200 BackgroundTrans c" IC, Chr(0xE7E8))
    PowerBtn.OnEvent("Click", ShowPowerMenu)
    PowerBtn.ToolTipText := "Power Options"

    ; Settings Gear (Bottom Right)
    if (!OpenedByHover) {
        CharmsBar.SetFont("s12 cGray", "Segoe MDL2 Assets")
        ConfigBtn := CharmsBar.Add("Text", "x" (BarWidth-35) " y+5 w30 h30 Center +0x200 BackgroundTrans", Chr(0xE713))
        ConfigBtn.OnEvent("Click", ShowSettings)
        ConfigBtn.ToolTipText := "Configure Bar & Theme"
    }
}

CheckTooltips(wParam, lParam, msg, hwnd) {
    Global LastToolTip
    if (!CharmsBar || hwnd != CharmsBar.Hwnd)
        return
    MouseGetPos(,, &WinHwnd, &CtrlHwnd)
    try {
        if (CtrlHwnd) {
            CtrlObj := GuiCtrlFromHwnd(CtrlHwnd)
            if (CtrlObj && HasProp(CtrlObj, "ToolTipText")) {
                if (LastToolTip != CtrlObj.ToolTipText) {
                    ToolTip(CtrlObj.ToolTipText)
                    LastToolTip := CtrlObj.ToolTipText
                    SetTimer () => ToolTip(), -2000
                }
                return
            }
        }
    }
    if (LastToolTip != "") {
        ToolTip()
        LastToolTip := ""
    }
}

AddCharm(IconCode, LabelText, Callback, TipText, TextColor) {
    IconCtl := CharmsBar.Add("Text", "Center w" BarWidth " h50 +0x200 BackgroundTrans c" TextColor, Chr(IconCode))
    IconCtl.OnEvent("Click", Callback)
    IconCtl.ToolTipText := TipText
    
    CharmsBar.SetFont("s10 c" TextColor, "Segoe UI")
    TextCtl := CharmsBar.Add("Text", "Center w" BarWidth " y+0 BackgroundTrans c" TextColor, LabelText)
    TextCtl.OnEvent("Click", Callback)
    TextCtl.ToolTipText := TipText
    
    CharmsBar.SetFont("s24", "Segoe MDL2 Assets") 
    CharmsBar.Add("Text", "h10 BackgroundTrans w" BarWidth)
}

AddDivider(Color) {
    CharmsBar.Add("Text", "x10 y+10 w" (BarWidth-20) " h1 Background555555") 
}

ExecuteSearch() {
    if (SearchProvider = "Windows")
        Send "#s"
    else {
        if FileExist(EverythingPath)
            Run EverythingPath
        else
            MsgBox "Everything.exe not found. Check settings."
    }
}

RunApp(Path) {
    try {
        Run Path
    } catch {
        MsgBox "Could not launch app:`n" Path
    }
}

GetBatteryInfo() {
    sps := Buffer(12, 0)
    DllCall("GetSystemPowerStatus", "Ptr", sps.Ptr)
    AC := NumGet(sps, 0, "UChar")
    Life := NumGet(sps, 2, "UChar")
    return (Life == 255) ? "AC Power" : ((AC?"Charging ":"Battery ") Life "%")
}

ShowPowerMenu(*) {
    PMenu := Menu()
    PMenu.Add("Sleep", (*) => DllCall("PowrProf\SetSuspendState", "Int", 0, "Int", 0, "Int", 0))
    PMenu.Add("Restart", (*) => Shutdown(2))
    PMenu.Add("Shut Down", (*) => Shutdown(1))
    PMenu.Show()
}

GetThemeColors() {
    if (CurrentTheme = "Light")
        return {Bg: "F0F0F0", Text: "000000", Icon: "000000", Blur: 220}
    else if (CurrentTheme = "Custom")
        return {Bg: CustomColor, Text: "FFFFFF", Icon: "FFFFFF", Blur: 210}
    else 
        ; Default Dark (No Blur, just dark gray/black)
        return {Bg: "101010", Text: "FFFFFF", Icon: "FFFFFF", Blur: 230}
}

ShowSettings(*) {
    Global ConfigGui, SearchProvider, EverythingPath, CurrentTheme, CustomColor
    Global AppA_Name, AppA_Path, AppB_Name, AppB_Path, AppC_Name, AppC_Path
    
    HideCharms()
    ConfigGui := Gui("+AlwaysOnTop", "Charms Settings")
    ConfigGui.SetFont("s9", "Segoe UI")

    ConfigGui.Add("GroupBox", "w320 h90 section", "Theme")
    RadDark := ConfigGui.Add("Radio", "xs+10 ys+25 Checked" (CurrentTheme="Dark"), "Dark (Glass)")
    RadLight := ConfigGui.Add("Radio", "x+10 yp Checked" (CurrentTheme="Light"), "Light")
    RadCustom := ConfigGui.Add("Radio", "x+10 yp Checked" (CurrentTheme="Custom"), "Custom Hex:")
    EdtHex := ConfigGui.Add("Edit", "x+5 yp-3 w60", CustomColor)

    ConfigGui.Add("GroupBox", "xs y+20 w320 h80", "Search")
    RadWin := ConfigGui.Add("Radio", "xs+10 yp+25 Checked" (SearchProvider="Windows"), "Windows")
    RadEvt := ConfigGui.Add("Radio", "x+10 yp Checked" (SearchProvider="Everything"), "Everything")
    ConfigGui.Add("Text", "xs+10 y+5 cGray", "Everything Path:")
    EdtEvt := ConfigGui.Add("Edit", "x+5 yp-3 w200", EverythingPath)

    ConfigGui.Add("GroupBox", "xs y+20 w320 h160", "Custom Apps (A, B, C)")
    
    AddAppRow(Label, NameVar, PathVar, YPos) {
        ConfigGui.Add("Text", "xs+10 y" YPos " w20", Label)
        E_Name := ConfigGui.Add("Edit", "x+5 yp-3 w80", NameVar)
        E_Path := ConfigGui.Add("Edit", "x+5 yp w140", PathVar)
        Btn := ConfigGui.Add("Button", "x+5 yp w40", "...")
        Btn.OnEvent("Click", (*) => (Sel := FileSelect(3,, "Select App"), (Sel && E_Path.Value := Sel)))
        return [E_Name, E_Path]
    }

    CtrlsA := AddAppRow("A:", AppA_Name, AppA_Path, "p+25")
    CtrlsB := AddAppRow("B:", AppB_Name, AppB_Path, "+10")
    CtrlsC := AddAppRow("C:", AppC_Name, AppC_Path, "+10")

    BtnSave := ConfigGui.Add("Button", "xs y+20 w320 h30", "Save Settings")
    BtnSave.OnEvent("Click", SaveAll)

    ConfigGui.Show()

    SaveAll(*) {
        if RadDark.Value 
            Global CurrentTheme := "Dark"
        else if RadLight.Value
            Global CurrentTheme := "Light"
        else
            Global CurrentTheme := "Custom"
        
        Global CustomColor := EdtHex.Value
        Global SearchProvider := RadWin.Value ? "Windows" : "Everything"
        Global EverythingPath := EdtEvt.Value
        Global AppA_Name := CtrlsA[1].Value, AppA_Path := CtrlsA[2].Value
        Global AppB_Name := CtrlsB[1].Value, AppB_Path := CtrlsB[2].Value
        Global AppC_Name := CtrlsC[1].Value, AppC_Path := CtrlsC[2].Value

        IniWrite(CurrentTheme, IniFile, "Theme", "Mode")
        IniWrite(CustomColor, IniFile, "Theme", "CustomColor")
        IniWrite(SearchProvider, IniFile, "Settings", "SearchProvider")
        IniWrite(EverythingPath, IniFile, "Paths", "EverythingPath")
        IniWrite(AppA_Name, IniFile, "AppA", "Name"), IniWrite(AppA_Path, IniFile, "AppA", "Path")
        IniWrite(AppB_Name, IniFile, "AppB", "Name"), IniWrite(AppB_Path, IniFile, "AppB", "Path")
        IniWrite(AppC_Name, IniFile, "AppC", "Name"), IniWrite(AppC_Path, IniFile, "AppC", "Path")

        MsgBox("Saved!", "Charms", "T1")
        ConfigGui.Destroy()
        ConfigGui := ""
    }
}