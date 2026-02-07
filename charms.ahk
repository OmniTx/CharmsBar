#Requires AutoHotkey v2.0
; ==============================================================================
; Script: Charms Bar Platinum v21 (Static Gradient / No Animation)
; Features: Search Config | Static Multi-Point Gradient | Image BG | No UAC
; ==============================================================================

; --- FORCE ADMIN ---
if not A_IsAdmin {
    try Run("*RunAs `"" A_ScriptFullPath "`"")
    ExitApp
}

; --- Configuration Defaults ---
Global BaseWidth := 160  
Global HoverDelay := 400
Global IniFile := A_ScriptDir "\CharmsSettings.ini"

; --- Load Settings ---
Global SearchProvider := IniRead(IniFile, "Settings", "SearchProvider", "Windows")
Global EverythingPath := IniRead(IniFile, "Paths", "EverythingPath", "C:\Program Files\Everything 1.5a\Everything.exe")
Global LaunchOnBoot   := IniRead(IniFile, "Settings", "LaunchOnBoot", "0")

; Theme Settings
Global BackgroundMode := IniRead(IniFile, "Theme", "BackgroundMode", "Dark") 
Global CustomColor    := IniRead(IniFile, "Theme", "CustomColor", "101010")
Global BgImage        := IniRead(IniFile, "Theme", "BgImage", "")
Global GradientStr    := IniRead(IniFile, "Theme", "GradientStr", "2E3192,1BFFFF")
Global ForceText      := IniRead(IniFile, "Theme", "ForceText", "Auto") 

; Apps
Global AppA_Name := IniRead(IniFile, "AppA", "Name", "Notepad")
Global AppA_Path := IniRead(IniFile, "AppA", "Path", "notepad.exe")
Global AppB_Name := IniRead(IniFile, "AppB", "Name", "File Explorer")
Global AppB_Path := IniRead(IniFile, "AppB", "Path", "explorer.exe")
Global AppC_Name := IniRead(IniFile, "AppC", "Name", "Cmd")
Global AppC_Path := IniRead(IniFile, "AppC", "Path", "cmd.exe")

; --- State Variables ---
Global CharmsBar := "", ConfigGui := ""
Global OpenedByHover := false, CornerHoverStart := 0
Global TimeCtl := "", AmPmCtl := "", DateCtl := "", VolTextCtl := ""
Global BtnA := "", BtnB := "", BtnC := "" 
Global LastToolTip := "", Hue := 0 
Global MonLeft := 0, MonTop := 0, MonRight := 0, MonBottom := 0
Global CurrentBarWidth := 160
Global hGradientBitmap := 0, PicBg := ""

; Start Watchers
SetTimer(HotCornerWatch, 100)
OnMessage(0x0200, CheckTooltips)

#c::ToggleCharmsBar(false)

; ==============================================================================
; HELPER: RUN AS USER
; ==============================================================================
RunAsUser(Target, Args := "") {
    if GetKeyState("Shift", "P") {
        try Run(Target " " Args)
        return
    }
    try {
        Shell := ComObject("Shell.Application")
        Desktop := Shell.Windows.FindWindowSW(0, 0, 8, 0, 1) 
        if (Desktop)
            Desktop.Document.Application.ShellExecute(Target, Args)
        else
            Run(Target " " Args)
    } catch {
        Run(Target " " Args)
    }
}

; ==============================================================================
; HELPER: GENERATE STATIC GRADIENT BITMAP
; ==============================================================================
CreateGradientBitmap(HexList, Width, Height) {
    Colors := StrSplit(StrReplace(HexList, " ", ""), ",")
    if (Colors.Length < 2)
        Colors.Push(Colors[1]) 

    BufSize := Height * 4
    PixelData := Buffer(BufSize, 0)
    
    NumSegments := Colors.Length - 1
    SegmentHeight := Height / NumSegments
    
    Loop NumSegments {
        Idx := A_Index
        Hex1 := StrReplace(Colors[Idx], "#", "")
        Hex2 := StrReplace(Colors[Idx+1], "#", "")
        
        if (StrLen(Hex1) != 6)
            Hex1 := "000000"
        if (StrLen(Hex2) != 6)
            Hex2 := "000000"

        R1 := Integer("0x" SubStr(Hex1,1,2)), G1 := Integer("0x" SubStr(Hex1,3,2)), B1 := Integer("0x" SubStr(Hex1,5,2))
        R2 := Integer("0x" SubStr(Hex2,1,2)), G2 := Integer("0x" SubStr(Hex2,3,2)), B2 := Integer("0x" SubStr(Hex2,5,2))
        
        StartY := Round((Idx-1) * SegmentHeight)
        EndY   := Round(Idx * SegmentHeight)
        Steps  := EndY - StartY
        
        Loop Steps {
            Y := A_Index - 1
            Factor := Y / Steps
            R := Round(R1 + (R2 - R1) * Factor)
            G := Round(G1 + (G2 - G1) * Factor)
            B := Round(B1 + (B2 - B1) * Factor)
            
            Offset := ((StartY + Y) * 4)
            if (Offset < BufSize) {
                NumPut("UChar", B, PixelData, Offset)
                NumPut("UChar", G, PixelData, Offset+1)
                NumPut("UChar", R, PixelData, Offset+2)
                NumPut("UChar", 255, PixelData, Offset+3)
            }
        }
    }

    HBM := DllCall("CreateBitmap", "Int", 1, "Int", Height, "UInt", 1, "UInt", 32, "Ptr", PixelData.Ptr, "Ptr")
    return HBM
}

; ==============================================================================
; CORE LOGIC & GUI
; ==============================================================================

ToggleCharmsBar(fromHover := false) {
    Global CharmsBar, OpenedByHover
    Global MonLeft, MonTop, MonRight, MonBottom, CurrentBarWidth

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

    BarH := MonBottom - MonTop
    BarX := MonRight - CurrentBarWidth
    BarY := MonTop

    CharmsBar.Show("x" BarX " y" BarY " w" CurrentBarWidth " h" BarH " NoActivate Hide")
    DllCall("AnimateWindow", "Ptr", CharmsBar.Hwnd, "Int", 150, "Int", 0x40000 | 0x00002)
    
    try WinActivate("ahk_id " CharmsBar.Hwnd)
    
    SetTimer(CheckFocus, 100)
    SetTimer(UpdateClock, 1000)
    SetTimer(RainbowCycle, 50)
}

CreateCharmsBar() {
    Global CharmsBar, TimeCtl, AmPmCtl, DateCtl, BtnA, BtnB, BtnC, VolTextCtl
    Global hGradientBitmap, PicBg
    Global MonLeft, MonTop, MonRight, MonBottom, CurrentBarWidth

    GetMonitorUnderMouse(&MonLeft, &MonTop, &MonRight, &MonBottom)
    MonW := MonRight - MonLeft
    Scale := Max(1.0, MonW / 1920)
    CurrentBarWidth := Round(BaseWidth * Scale)
    BarH := MonBottom - MonTop
    
    FS_Huge := Round(24 * Scale), FS_Big := Round(26 * Scale)
    FS_Med := Round(12 * Scale), FS_Sml := Round(10 * Scale)

    ; --- DETERMINE COLORS ---
    if (BackgroundMode == "Dark") {
        BgColor := "000000"
        TextColor := "FFFFFF"
        SubText := "BBBBBB"
    } else if (BackgroundMode == "Light") {
        BgColor := "F0F0F0"
        TextColor := "000000"
        SubText := "555555"
    } else {
        BgColor := (BackgroundMode == "Custom") ? CustomColor : "000000"
        
        if (ForceText == "Black")
            TextColor := "000000"
        else if (ForceText == "White")
            TextColor := "FFFFFF"
        else if (BackgroundMode == "Custom")
            TextColor := GetContrastingColor(BgColor)
        else
            TextColor := "FFFFFF" 
            
        SubText := (TextColor == "FFFFFF") ? "CCCCCC" : "444444"
    }

    CharmsBar := Gui("-Caption +AlwaysOnTop +ToolWindow +Owner +E0x08000000", "CharmsBar")
    CharmsBar.BackColor := BgColor
    CharmsBar.MarginX := 0, CharmsBar.MarginY := 0
    WinSetTransparent(235, CharmsBar) 
    CharmsBar.OnEvent("Escape", (*) => HideCharms())

    ; --- BACKGROUND LAYER ---
    if (BackgroundMode == "Image" && FileExist(BgImage)) {
        try CharmsBar.Add("Picture", "x0 y0 w" CurrentBarWidth " h" BarH " 0x4000000", BgImage)
    } 
    else if (BackgroundMode == "Gradient") {
        hGradientBitmap := CreateGradientBitmap(GradientStr, 1, BarH)
        PicBg := CharmsBar.Add("Picture", "x0 y0 w" CurrentBarWidth " h" BarH " 0x4000000")
        try PicBg.Value := "*w" CurrentBarWidth " *h" BarH " HBITMAP:" hGradientBitmap
    }

    ; --- HEADER ---
    CharmsBar.SetFont("s" FS_Med " c" TextColor, "Segoe UI Semibold")
    CharmsBar.Add("Text", "x0 y10 w" CurrentBarWidth " Center BackgroundTrans", A_UserName)
    CharmsBar.SetFont("s" FS_Sml " c" SubText, "Segoe UI") 
    CharmsBar.Add("Text", "x0 y+2 w" CurrentBarWidth " Center BackgroundTrans", GetBatteryInfo())
    AddDivider(TextColor)

    ; --- CHARMS ---
    CharmsBar.SetFont("s" FS_Huge " c" TextColor, "Segoe MDL2 Assets")
    AddCharm(0xE721, "Search", (*) => (HideCharms(), ExecuteSearch()), "Search System/Files", TextColor, FS_Sml)
    AddCharm(0xF0E3, "Clipboard", (*) => (HideCharms(), Send("#v")), "Clipboard History (Win+V)", TextColor, FS_Sml)
    AddCharm(0xE80F, "Start", (*) => (HideCharms(), Send("{LWin}")), "Open Start Menu", TextColor, FS_Sml)
    AddCharm(0xE772, "Devices", (*) => (HideCharms(), Send("#k")), "Connect Devices (Win+K)", TextColor, FS_Sml)
    AddCharm(0xE713, "Settings", (*) => (HideCharms(), Send("#i")), "Windows Settings", TextColor, FS_Sml)

    AddDivider(TextColor)

    ; --- MEDIA ---
    CharmsBar.SetFont("s" FS_Sml " c" SubText, "Segoe UI")
    CharmsBar.Add("Text", "x0 y+10 w" CurrentBarWidth " Center BackgroundTrans", "MEDIA")
    CharmsBar.SetFont("s" Round(16*Scale) " c" TextColor, "Segoe MDL2 Assets")
    BW := Round(40 * Scale)
    StartX := (CurrentBarWidth - (BW * 3)) / 2
    
    PrevBtn := CharmsBar.Add("Text", "x" StartX " y+5 w" BW " h30 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE892))
    PrevBtn.OnEvent("Click", (*) => Send("{Media_Prev}"))
    PlayBtn := CharmsBar.Add("Text", "x+0 w" BW " h30 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE768))
    PlayBtn.OnEvent("Click", (*) => Send("{Media_Play_Pause}"))
    NextBtn := CharmsBar.Add("Text", "x+0 w" BW " h30 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE893))
    NextBtn.OnEvent("Click", (*) => Send("{Media_Next}"))

    CharmsBar.SetFont("s" FS_Sml " c" TextColor, "Segoe UI")
    VolSlider := CharmsBar.Add("Slider", "x5 y+10 w" (CurrentBarWidth-40) " h20 ToolTip NoTicks Background" BgColor, SoundGetVolume())
    VolSlider.OnEvent("Change", UpdateVolumeLabel)
    VolTextCtl := CharmsBar.Add("Text", "x+0 yp w35 Right BackgroundTrans c" TextColor, Round(SoundGetVolume()) "%")

    AddDivider(TextColor)

    ; --- TOOLS ---
    CharmsBar.SetFont("s" FS_Sml " c" SubText, "Segoe UI")
    CharmsBar.Add("Text", "x0 y+10 w" CurrentBarWidth " Center BackgroundTrans", "TOOLS")
    CharmsBar.SetFont("s" Round(14*Scale) " c" TextColor, "Segoe MDL2 Assets")
    SnipBtn := CharmsBar.Add("Text", "x20 y+5 w40 h40 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE70F))
    SnipBtn.OnEvent("Click", (*) => (HideCharms(), Send("#+s")))
    CalcBtn := CharmsBar.Add("Text", "x+0 w40 h40 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE8EF))
    CalcBtn.OnEvent("Click", (*) => (HideCharms(), RunAsUser("calc.exe")))
    TaskBtn := CharmsBar.Add("Text", "x+0 w40 h40 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE9D9))
    TaskBtn.OnEvent("Click", (*) => (HideCharms(), Run("taskmgr.exe")))

    AddDivider(TextColor)

    ; --- APPS ---
    CharmsBar.SetFont("s" FS_Sml " c" SubText, "Segoe UI")
    CharmsBar.Add("Text", "x0 y+10 w" CurrentBarWidth " Center BackgroundTrans", "APPS")
    CharmsBar.SetFont("s" Round(12*Scale) " w700", "Segoe UI") 
    BtnA := CharmsBar.Add("Text", "x20 y+5 w40 h40 Center +0x200 BackgroundTrans", "A")
    BtnA.OnEvent("Click", (*) => (HideCharms(), RunAsUser(AppA_Path)))
    BtnB := CharmsBar.Add("Text", "x+0 w40 h40 Center +0x200 BackgroundTrans", "B")
    BtnB.OnEvent("Click", (*) => (HideCharms(), RunAsUser(AppB_Path)))
    BtnC := CharmsBar.Add("Text", "x+0 w40 h40 Center +0x200 BackgroundTrans", "C")
    BtnC.OnEvent("Click", (*) => (HideCharms(), RunAsUser(AppC_Path)))

    ; --- BOTTOM ---
    BottomY := BarH - 140 
    CharmsBar.SetFont("s" FS_Big " c" TextColor, "Segoe UI Light")
    W_Time := Round(CurrentBarWidth * 0.6)
    TimeCtl := CharmsBar.Add("Text", "x0 y" BottomY " w" W_Time " Right BackgroundTrans", FormatTime(, "h:mm"))
    CharmsBar.SetFont("s" FS_Sml " c" TextColor, "Segoe UI")
    AmPmCtl := CharmsBar.Add("Text", "x+5 yp+12 w50 Left BackgroundTrans", FormatTime(, "tt"))
    CharmsBar.SetFont("s" FS_Sml " c" SubText, "Segoe UI")
    CharmsBar.Add("Text", "x0 y+10 w" CurrentBarWidth " Center BackgroundTrans", FormatTime(, "M/d/yyyy"))

    CharmsBar.SetFont("s" Round(20*Scale) " c" TextColor, "Segoe MDL2 Assets")
    PowerBtn := CharmsBar.Add("Text", "x0 y+10 w" CurrentBarWidth " h40 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE7E8))
    PowerBtn.OnEvent("Click", ShowPowerMenu)

    if (!OpenedByHover) {
        CharmsBar.SetFont("s" FS_Med " c" TextColor, "Segoe MDL2 Assets")
        ConfigBtn := CharmsBar.Add("Text", "x" (CurrentBarWidth-35) " y+5 w30 h30 Center +0x200 BackgroundTrans", Chr(0xE713))
        ConfigBtn.OnEvent("Click", ShowSettings)
    }
}

; ==============================================================================
; SETTINGS GUI
; ==============================================================================
ShowSettings(*) {
    Global ConfigGui, SearchProvider, EverythingPath, BackgroundMode, CustomColor, LaunchOnBoot, BgImage, GradientStr, ForceText
    Global AppA_Name, AppA_Path, AppB_Name, AppB_Path, AppC_Name, AppC_Path
    
    HideCharms()
    ConfigGui := Gui("+AlwaysOnTop", "Charms Settings")
    ConfigGui.SetFont("s9", "Segoe UI")

    ConfigGui.Add("Checkbox", "vCbBoot Checked" LaunchOnBoot, "Run at Startup (No UAC)")

    ConfigGui.Add("GroupBox", "w340 h150 section", "Appearance")
    ConfigGui.Add("Text", "xs+10 ys+25", "Mode:")
    DDLMode := ConfigGui.Add("DropDownList", "x+10 yp-3 w120 vDDLMode Choose" (BackgroundMode="Dark"?1:BackgroundMode="Light"?2:BackgroundMode="Custom"?3:BackgroundMode="Image"?4:5), ["Dark","Light","Custom","Image","Gradient"])
    
    ConfigGui.Add("Text", "xs+10 y+10", "Solid Hex:")
    E_Hex := ConfigGui.Add("Edit", "x+10 yp-3 w80", CustomColor)
    
    ConfigGui.Add("Text", "xs+10 y+10", "Image:")
    E_Img := ConfigGui.Add("Edit", "x+10 yp-3 w160", BgImage)
    BtnImg := ConfigGui.Add("Button", "x+5 yp-1 w30", "...")
    BtnImg.OnEvent("Click", (*) => (Sel := FileSelect(3,, "Select Image", "Images (*.jpg; *.png; *.bmp)"), (Sel && E_Img.Value := Sel)))

    ConfigGui.Add("Text", "xs+10 y+10", "Gradient:")
    E_Grad := ConfigGui.Add("Edit", "x+10 yp-3 w195", GradientStr)
    
    ConfigGui.Add("Text", "xs+10 y+10", "Text Color:")
    DDLText := ConfigGui.Add("DropDownList", "x+10 yp-3 w80 Choose" (ForceText="Auto"?1:ForceText="White"?2:3), ["Auto","White","Black"])

    ; --- RESTORED SEARCH SETTINGS ---
    ConfigGui.Add("GroupBox", "xs y+20 w340 h85", "Search Provider")
    RadWin := ConfigGui.Add("Radio", "xs+10 yp+25 Checked" (SearchProvider="Windows"), "Windows Search")
    RadEvt := ConfigGui.Add("Radio", "x+10 yp Checked" (SearchProvider="Everything"), "Everything Search")
    ConfigGui.Add("Text", "xs+10 y+5 cGray", "Path:")
    EdtEvt := ConfigGui.Add("Edit", "x+5 yp-3 w240", EverythingPath)
    BtnEvt := ConfigGui.Add("Button", "x+5 yp-1 w30", "...")
    BtnEvt.OnEvent("Click", (*) => (Sel := FileSelect(3,, "Select Everything.exe", "Executables (*.exe)"), (Sel && EdtEvt.Value := Sel)))

    ConfigGui.Add("GroupBox", "xs y+20 w340 h160", "Custom Apps (A, B, C)")
    
    AddAppRow(Label, NameVar, PathVar, YPos) {
        ConfigGui.Add("Text", "xs+10 y" YPos " w20", Label)
        E_Name := ConfigGui.Add("Edit", "x+5 yp-3 w80", NameVar)
        E_Path := ConfigGui.Add("Edit", "x+5 yp w160", PathVar)
        Btn := ConfigGui.Add("Button", "x+5 yp w30", "...")
        Btn.OnEvent("Click", (*) => (Sel := FileSelect(3,, "Select App"), (Sel && E_Path.Value := Sel)))
        return [E_Name, E_Path]
    }

    CtrlsA := AddAppRow("A:", AppA_Name, AppA_Path, "p+25")
    CtrlsB := AddAppRow("B:", AppB_Name, AppB_Path, "+10")
    CtrlsC := AddAppRow("C:", AppC_Name, AppC_Path, "+10")

    BtnSave := ConfigGui.Add("Button", "xs y+20 w340 h30", "Save & Apply")
    BtnSave.OnEvent("Click", SaveAll)

    ConfigGui.Show()

    SaveAll(*) {
        Global BackgroundMode := DDLMode.Text
        Global CustomColor := StrReplace(E_Hex.Value, "#", "")
        Global BgImage := E_Img.Value
        Global GradientStr := E_Grad.Value
        Global ForceText := DDLText.Text
        Global LaunchOnBoot := ConfigGui["CbBoot"].Value
        
        Global SearchProvider := RadWin.Value ? "Windows" : "Everything"
        Global EverythingPath := EdtEvt.Value
        
        Global AppA_Name := CtrlsA[1].Value, AppA_Path := CtrlsA[2].Value
        Global AppB_Name := CtrlsB[1].Value, AppB_Path := CtrlsB[2].Value
        Global AppC_Name := CtrlsC[1].Value, AppC_Path := CtrlsC[2].Value

        SetStartup(LaunchOnBoot)

        IniWrite(LaunchOnBoot, IniFile, "Settings", "LaunchOnBoot")
        IniWrite(SearchProvider, IniFile, "Settings", "SearchProvider")
        IniWrite(EverythingPath, IniFile, "Paths", "EverythingPath")
        IniWrite(BackgroundMode, IniFile, "Theme", "BackgroundMode")
        IniWrite(CustomColor, IniFile, "Theme", "CustomColor")
        IniWrite(BgImage, IniFile, "Theme", "BgImage")
        IniWrite(GradientStr, IniFile, "Theme", "GradientStr")
        IniWrite(ForceText, IniFile, "Theme", "ForceText")
        
        IniWrite(AppA_Name, IniFile, "AppA", "Name"), IniWrite(AppA_Path, IniFile, "AppA", "Path")
        IniWrite(AppB_Name, IniFile, "AppB", "Name"), IniWrite(AppB_Path, IniFile, "AppB", "Path")
        IniWrite(AppC_Name, IniFile, "AppC", "Name"), IniWrite(AppC_Path, IniFile, "AppC", "Path")

        MsgBox("Saved!", "Charms", "T1")
        ConfigGui.Destroy()
        ConfigGui := ""
    }
}

; --- OTHER HELPERS ---
HideCharms() {
    Global CharmsBar, hGradientBitmap
    SetTimer(CheckFocus, 0), SetTimer(UpdateClock, 0), SetTimer(RainbowCycle, 0)
    ToolTip()
    if (!CharmsBar || !WinExist("ahk_id " CharmsBar.Hwnd))
        return
    DllCall("AnimateWindow", "Ptr", CharmsBar.Hwnd, "Int", 150, "Int", 0x10000 | 0x40000 | 0x00001)
    if (hGradientBitmap) {
        DllCall("DeleteObject", "Ptr", hGradientBitmap)
        hGradientBitmap := 0
    }
    CharmsBar.Destroy(), CharmsBar := ""
}

GetContrastingColor(Hex) {
    Hex := StrReplace(Hex, "#", "")
    if (StrLen(Hex) != 6)
        return "FFFFFF" 
    R := Integer("0x" SubStr(Hex, 1, 2)), G := Integer("0x" SubStr(Hex, 3, 2)), B := Integer("0x" SubStr(Hex, 5, 2))
    return (((R*299) + (G*587) + (B*114)) / 1000 >= 128) ? "000000" : "FFFFFF"
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
        ToolTip(), LastToolTip := ""
    }
}

AddCharm(IconCode, LabelText, Callback, TipText, TextColor, FontSize) {
    IconCtl := CharmsBar.Add("Text", "Center w" CurrentBarWidth " h50 +0x200 BackgroundTrans c" TextColor, Chr(IconCode))
    IconCtl.OnEvent("Click", Callback), IconCtl.ToolTipText := TipText
    CharmsBar.SetFont("s" FontSize " c" TextColor, "Segoe UI")
    TextCtl := CharmsBar.Add("Text", "Center w" CurrentBarWidth " y+0 BackgroundTrans c" TextColor, LabelText)
    TextCtl.OnEvent("Click", Callback), TextCtl.ToolTipText := TipText
    CharmsBar.SetFont("s24", "Segoe MDL2 Assets") 
    CharmsBar.Add("Text", "h10 BackgroundTrans w" CurrentBarWidth)
}

AddDivider(Color) {
    CharmsBar.Add("Text", "x10 y+10 w" (CurrentBarWidth-20) " h1 Background555555") 
}

ExecuteSearch() {
    if (SearchProvider = "Windows")
        Send("#s")
    else if FileExist(EverythingPath)
        RunAsUser(EverythingPath)
    else
        MsgBox("Everything.exe not found.")
}

GetBatteryInfo() {
    sps := Buffer(12, 0)
    DllCall("GetSystemPowerStatus", "Ptr", sps.Ptr)
    Life := NumGet(sps, 2, "UChar")
    return (Life == 255) ? "AC Power" : ((NumGet(sps, 0, "UChar")?"Charging ":"Battery ") Life "%")
}

ShowPowerMenu(*) {
    PMenu := Menu()
    PMenu.Add("Sleep", (*) => DllCall("PowrProf\SetSuspendState", "Int", 0, "Int", 0, "Int", 0))
    PMenu.Add("Restart", (*) => Shutdown(2))
    PMenu.Add("Shut Down", (*) => Shutdown(1))
    PMenu.Show()
}

GetMonitorUnderMouse(&ML, &MT, &MR, &MB) {
    CoordMode "Mouse", "Screen"
    MouseGetPos &MX, &MY
    Loop MonitorGetCount() {
        MonitorGet(A_Index, &L, &T, &R, &B)
        if (MX >= L && MX < R && MY >= T && MY < B) {
            ML := L, MT := T, MR := R, MB := B
            return
        }
    }
    MonitorGet(MonitorGetPrimary(), &ML, &MT, &MR, &MB)
}

CalculateScale(MonWidth) => Max(1.0, MonWidth / 1920)

IsWindowFullScreen(winID) {
    if !winID
        return false
    if (WinGetClass("ahk_id " winID) ~= "Progman|WorkerW")
        return false
    WinGetPos(&WinX, &WinY, &WinW, &WinH, "ahk_id " winID)
    MonitorHandle := DllCall("MonitorFromWindow", "Ptr", winID, "UInt", 0x2) 
    MonInfo := Buffer(40), NumPut("UInt", 40, MonInfo)
    DllCall("GetMonitorInfo", "Ptr", MonitorHandle, "Ptr", MonInfo)
    return (WinX <= NumGet(MonInfo, 4, "Int") && WinY <= NumGet(MonInfo, 8, "Int") && WinW >= (NumGet(MonInfo, 12, "Int") - NumGet(MonInfo, 4, "Int")) && WinH >= (NumGet(MonInfo, 16, "Int") - NumGet(MonInfo, 8, "Int")))
}

SetStartup(Enable) {
    TaskName := "CharmsBarAutoStart"
    if (Enable)
        RunWait('schtasks /Create /TN "' TaskName '" /TR "' A_ScriptFullPath '" /SC ONLOGON /RL HIGHEST /F',, "Hide")
    else
        RunWait('schtasks /Delete /TN "' TaskName '" /F',, "Hide")
}

HotCornerWatch() {
    Global OpenedByHover, CharmsBar, CornerHoverStart, MonLeft, MonTop, MonRight, MonBottom
    if (IsWindowFullScreen(WinActive("A"))) {
        CornerHoverStart := 0
        return
    }
    CoordMode "Mouse", "Screen"
    MouseGetPos &MX, &MY
    MonitorGet(MonitorGetPrimary(), &PL, &PT, &PR, &PB)
    InCorner := (MX >= (PR - 10) && MY <= (PT + 10))
    if (InCorner && !CharmsBar) {
        if (CornerHoverStart == 0)
            CornerHoverStart := A_TickCount
        else if (A_TickCount - CornerHoverStart > HoverDelay)
            OpenedByHover := true, ToggleCharmsBar(true), CornerHoverStart := 0 
    } else
        CornerHoverStart := 0
    if (CharmsBar && OpenedByHover && MX < (MonRight - CurrentBarWidth))
        HideCharms(), OpenedByHover := false
}

CheckFocus() {
    Global CharmsBar, ConfigGui
    if (ConfigGui && WinActive("ahk_id " ConfigGui.Hwnd))
        return
    if (CharmsBar && !WinActive("ahk_id " CharmsBar.Hwnd))
        HideCharms(), OpenedByHover := false
}

RainbowCycle() {
    Global Hue, BtnA, BtnB, BtnC, CharmsBar
    if (!CharmsBar || !BtnA) 
        return
    Hue := Mod(Hue + 4, 360) 
    try BtnA.Opt("c" HSVtoRGB(Hue,1,1)), BtnB.Opt("c" HSVtoRGB(Mod(Hue+60,360),1,1)), BtnC.Opt("c" HSVtoRGB(Mod(Hue+120,360),1,1))
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
    Global TimeCtl, AmPmCtl
    if (TimeCtl)
        TimeCtl.Text := FormatTime(,"h:mm"), AmPmCtl.Text := FormatTime(,"tt")
}

UpdateVolumeLabel(GuiCtrl, *) {
    SoundSetVolume(GuiCtrl.Value)
    VolTextCtl.Text := Round(GuiCtrl.Value) "%"
}

#!c:: 
{
    if !WinExist("A")
        return
    WinGetPos(,, &WinW, &WinH, "A")
    GetMonitorUnderMouse(&ML, &MT, &MR, &MB)
    try WinMove(ML + (MR - ML - WinW) / 2, MT + (MB - MT - WinH) / 2,,, "A")
}