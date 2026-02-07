#Requires AutoHotkey v2.0
; ==============================================================================
; Script: Charms Bar Platinum v23 (Static Gradient / Slide-in Animation)
; Features: Search Config | Static Multi-Point Gradient | Image BG | No UAC
; ==============================================================================

; --- FORCE ADMIN (Removed to prevent blocking other apps) ---
; Script will run with User privileges by default unless elevated manually
if not A_IsAdmin {
    try Run("*RunAs `"" A_ScriptFullPath "`"")
    ExitApp
}

; --- Configuration Defaults ---
Global BaseWidth := 160  
Global HoverDelay := 400
Global IniFile := A_ScriptDir "\CharmsSettings.ini"
Global CurrentVersion := "23"
Global UpdateJsonUrl := "https://raw.githubusercontent.com/omnitx/CharmsBar/main/update.json" ;
Global NewAhkUrl := ""
Global NewExeUrl := ""

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
Global VolumeStyle    := IniRead(IniFile, "Settings", "VolumeStyle", "Slider")

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
Global TimeCtl := "", AmPmCtl := "", DateCtl := "", VolTextCtl := "", VolBg := "", VolIcon := "", VolOverlay := ""
Global BtnA := "", BtnB := "", BtnC := "" 
Global LastToolTip := "", Hue := 0 
Global MonLeft := 0, MonTop := 0, MonRight := 0, MonBottom := 0
Global CurrentBarWidth := 160
Global hGradientBitmap := 0, PicBg := ""

; Start Watchers
SetTimer(HotCornerWatch, 100)
OnMessage(0x0200, CheckTooltips)
OnExit(ExitFunc)


#c::ToggleCharmsBar(false)

#!c::CenterActiveWindow()

; ==============================================================================
; AUTO-UPDATE FUNCTIONALITY
; ==============================================================================
CheckForUpdates(Interactive := false) {
    Global CurrentVersion, UpdateJsonUrl, NewAhkUrl, NewExeUrl
    
    try {
        ; Download update.json with cache busting
        whr := ComObject("WinHttp.WinHttpRequest.5.1")
        whr.Open("GET", UpdateJsonUrl "?t=" A_TickCount, true)
        whr.Send()
        whr.WaitForResponse()
        
        if (whr.Status != 200) {
            if (Interactive)
                MsgBox("Failed to check for updates. HTTP Status: " whr.Status, "Update Check", 0x10)
            return
        }
        
        JsonStr := whr.ResponseText
        
        ; Simple Regex Parsing for JSON
        VersionPat := '"version"\s*:\s*"([^"]+)"'
        AhkUrlPat  := '"ahk_url"\s*:\s*"([^"]+)"'
        ExeUrlPat  := '"exe_url"\s*:\s*"([^"]+)"'
        ChangelogPat := '"changelog"\s*:\s*"([^"]+)"'
        
        NewVersion := ""
        if RegExMatch(JsonStr, VersionPat, &Match)
            NewVersion := Match[1]
            
        if (NewVersion == "") {
            if (Interactive)
                MsgBox("Failed to parse update information.", "Update Check", 0x10)
            return
        }

        ; Extract URLs and Changelog
        if RegExMatch(JsonStr, AhkUrlPat, &Match)
            NewAhkUrl := Match[1]
        if RegExMatch(JsonStr, ExeUrlPat, &Match)
            NewExeUrl := Match[1]
        
        NewChangelog := "No details provided."
        if RegExMatch(JsonStr, ChangelogPat, &Match)
            NewChangelog := Match[1]
        
        if (NewVersion > CurrentVersion) {
            MsgText := "A new version is available!`n`n"
                     . "Current: " CurrentVersion "`n"
                     . "New: " NewVersion "`n`n"
                     . "Changes:`n" NewChangelog "`n`n"
                     . "Do you want to update now?"
            
            Result := MsgBox(MsgText, "Update Available", 0x4 + 0x40)
            if (Result = "Yes")
                UpdateScript()
        } else {
            if (Interactive)
                MsgBox("You are using the latest version (" CurrentVersion ").", "Update Check", 0x40)
        }
    } catch as err {
        if (Interactive)
            MsgBox("Failed to check for updates.`n`nError: " err.Message, "Update Check", 0x10)
    }
}

UpdateScript() {
    Global NewAhkUrl, NewExeUrl
    
    DownloadUrl := A_IsCompiled ? NewExeUrl : NewAhkUrl
    
    if (DownloadUrl == "") {
        MsgBox("Update URL not found for this version.", "Update Failed", 0x10)
        return
    }

    TempFile := A_ScriptDir "\update_temp" (A_IsCompiled ? ".exe" : ".ahk")
    
    try {
        Download(DownloadUrl, TempFile)
        
        if !FileExist(TempFile) {
            MsgBox("Download failed. The temporary file was not created.", "Update Failed", 0x10)
            return
        }
        
        ; Create batch file to replace the script/exe
        BatchFile := A_ScriptDir "\update.bat"
        if FileExist(BatchFile)
            FileDelete(BatchFile)
            
        ExePath := A_IsCompiled ? A_ScriptFullPath : A_AhkPath
        ScriptArgs := A_IsCompiled ? "" : ' "' A_ScriptFullPath '"'
        CurrentFileName := A_ScriptFullPath
        
        ; Batch file logic:
        ; 1. Wait for this process to end
        ; 2. Move temp file to current file (overwrite)
        ; 3. Restart the script/exe
        ; 4. Delete the batch file
        
        BatchCode := '@echo off`n'
        . 'timeout /t 1 /nobreak >nul`n'
        . ':loop`n'
        . 'tasklist | find /i "' A_ScriptName '" >nul`n'
        . 'if not errorlevel 1 (`n'
        . '    timeout /t 1 /nobreak >nul`n'
        . '    goto loop`n'
        . ')`n'
        . 'move /y "' TempFile '" "' CurrentFileName '" >nul`n'
        . 'start "" "' ExePath '"' ScriptArgs '`n'
        . 'del "%~f0" & exit'
        
        FileAppend(BatchCode, BatchFile)
        
        Run(BatchFile, , "Hide")
        ExitApp()
        
    } catch as err {
        MsgBox("Update failed!`n`nError: " err.Message, "Update Error", 0x10)
        if FileExist(TempFile)
            FileDelete(TempFile)
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
    Global CharmsBar, TimeCtl, AmPmCtl, DateCtl, BtnA, BtnB, BtnC, VolTextCtl, VolBg, VolIcon, VolOverlay
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

    CharmsBar := Gui("-Caption +AlwaysOnTop +ToolWindow +Owner +E0x08000000 -DPIScale", "CharmsBar")
    CharmsBar.BackColor := BgColor
    CharmsBar.MarginX := 0, CharmsBar.MarginY := 0
    WinSetTransparent(235, CharmsBar)
    
    CharmsBar.OnEvent("Escape", (*) => HideCharms())

    ; --- BACKGROUND LAYER ---
    if (BackgroundMode == "Image" && FileExist(BgImage)) {
        GetImageSize(BgImage, &ImgW, &ImgH)
        if (ImgW > 0 && ImgH > 0) {
            ; Calculate "Cover" aspect ratio
            ImgScale := Max(CurrentBarWidth / ImgW, BarH / ImgH)
            FinalW := Round(ImgW * ImgScale)
            FinalH := Round(ImgH * ImgScale)
            FinalX := Round((CurrentBarWidth - FinalW) / 2)
            FinalY := Round((BarH - FinalH) / 2)
            try CharmsBar.Add("Picture", "x" FinalX " y" FinalY " w" FinalW " h" FinalH " 0x4000000", BgImage)
        }
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
    AddCharm(0xE713, "Settings", (*) => (HideCharms(), Run("ms-settings:")), "Windows Settings", TextColor, FS_Sml)

    AddDivider(TextColor)

    ; --- MEDIA ---
    CharmsBar.SetFont("s" FS_Sml " c" SubText, "Segoe UI")
    CharmsBar.Add("Text", "x0 y+10 w" CurrentBarWidth " Center BackgroundTrans", "MEDIA")
    CharmsBar.SetFont("s" Round(16*Scale) " c" TextColor, "Segoe MDL2 Assets")
    BW := Round(40 * Scale)
    StartX := Round((CurrentBarWidth - (BW * 3)) / 2)
    
    PrevBtn := CharmsBar.Add("Text", "x" StartX " y+5 w" BW " h30 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE892))
    PrevBtn.OnEvent("Click", (*) => Send("{Media_Prev}"))
    PlayBtn := CharmsBar.Add("Text", "x+0 w" BW " h30 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE768))
    PlayBtn.OnEvent("Click", (*) => Send("{Media_Play_Pause}"))
    NextBtn := CharmsBar.Add("Text", "x+0 w" BW " h30 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE893))
    NextBtn.OnEvent("Click", (*) => Send("{Media_Next}"))

    CharmsBar.SetFont("s" FS_Med " c" TextColor, "Segoe MDL2 Assets")
    
    if (VolumeStyle = "Slider") {
        ; Modern Progress Bar Style
        ; Mute Icon
        VolIcon := CharmsBar.Add("Text", "x10 y+10 w30 h30 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE767))
        VolIcon.OnEvent("Click", (*) => (SoundSetMute(-1), UpdateVolumeLabel()))
        
        ; Clickable progress bar
        ; Calculate widths
        ; Total = CurrentBarWidth
        ; Icon (30) + Gap (5) + Bar (?) + Gap (5) + Text (35) + Gap (5)
        ; BarW = CurrentBarWidth - 30 - 35 - 20 = CurrentBarWidth - 85
        
        BarW := CurrentBarWidth - 85
        BarX := 45
        
        VolOverlay := CharmsBar.Add("Text", "x" BarX " yp+11 w" BarW " h8 Background333333")
        VolOverlay.OnEvent("Click", SetVolumeByClick)
        
        ; Progress bar fill (drawn on top)
        VolBg := CharmsBar.Add("Progress", "x" BarX " yp w" BarW " h8 Background333333 c" TextColor, SoundGetVolume())
        VolBg.Opt("-Smooth +E0x20")
        
        ; Volume percentage text
        CharmsBar.SetFont("s" FS_Sml " c" TextColor, "Segoe UI")
        VolTextCtl := CharmsBar.Add("Text", "x+5 yp-5 w35 h20 Right BackgroundTrans c" TextColor, Round(SoundGetVolume()) "%")
    } else {
        ; Buttons Mode (Default)
        VolDownBtn := CharmsBar.Add("Text", "x20 y+10 w40 h40 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE992))
        VolDownBtn.OnEvent("Click", (*) => (SoundSetVolume("-5"), UpdateVolumeLabel()))
        
        CharmsBar.SetFont("s" FS_Sml " c" TextColor, "Segoe UI")
        VolTextCtl := CharmsBar.Add("Text", "x+0 yp w60 h40 Center +0x200 BackgroundTrans c" TextColor, Round(SoundGetVolume()) "%")
        
        CharmsBar.SetFont("s" FS_Med " c" TextColor, "Segoe MDL2 Assets")
        VolUpBtn := CharmsBar.Add("Text", "x+0 yp w40 h40 Center +0x200 BackgroundTrans c" TextColor, Chr(0xE767))
        VolUpBtn.OnEvent("Click", (*) => (SoundSetVolume("+5"), UpdateVolumeLabel()))
    }

    AddDivider(TextColor)

    ; --- TOOLS ---
    CharmsBar.SetFont("s" FS_Sml " c" SubText, "Segoe UI")
    CharmsBar.Add("Text", "x0 y+10 w" CurrentBarWidth " Center BackgroundTrans", "TOOLS")
    CharmsBar.SetFont("s" Round(14*Scale) " c" TextColor, "Segoe MDL2 Assets")
    
    BtnSize := Round(40 * Scale)
    SideMargin := Round((CurrentBarWidth - (BtnSize * 3)) / 2)
    
    SnipBtn := CharmsBar.Add("Text", "x" SideMargin " y+5 w" BtnSize " h" BtnSize " Center +0x200 BackgroundTrans c" TextColor, Chr(0xE70F))
    SnipBtn.OnEvent("Click", (*) => (HideCharms(), Send("#+s")))
    CalcBtn := CharmsBar.Add("Text", "x+0 w" BtnSize " h" BtnSize " Center +0x200 BackgroundTrans c" TextColor, Chr(0xE8EF))
    CalcBtn.OnEvent("Click", (*) => (HideCharms(), RunAsUser("calc.exe")))
    TaskBtn := CharmsBar.Add("Text", "x+0 w" BtnSize " h" BtnSize " Center +0x200 BackgroundTrans c" TextColor, Chr(0xE9D9))
    TaskBtn.OnEvent("Click", (*) => (HideCharms(), Run("taskmgr.exe")))

    AddDivider(TextColor)

    ; --- APPS ---
    CharmsBar.SetFont("s" FS_Sml " c" SubText, "Segoe UI")
    CharmsBar.Add("Text", "x0 y+10 w" CurrentBarWidth " Center BackgroundTrans", "APPS")
    CharmsBar.SetFont("s" Round(12*Scale) " w700", "Segoe UI")
    
    ; Reuse SideMargin/BtnSize from Tools or recalcluate if needed (they are same)
    BtnA := CharmsBar.Add("Text", "x" SideMargin " y+5 w" BtnSize " h" BtnSize " Center +0x200 BackgroundTrans", "A")
    BtnA.OnEvent("Click", (*) => (HideCharms(), RunAsUser(AppA_Path)))
    BtnB := CharmsBar.Add("Text", "x+0 w" BtnSize " h" BtnSize " Center +0x200 BackgroundTrans", "B")
    BtnB.OnEvent("Click", (*) => (HideCharms(), RunAsUser(AppB_Path)))
    BtnC := CharmsBar.Add("Text", "x+0 w" BtnSize " h" BtnSize " Center +0x200 BackgroundTrans", "C")
    BtnC.OnEvent("Click", (*) => (HideCharms(), RunAsUser(AppC_Path)))

    ; --- BOTTOM ---
    BottomY := BarH - 180 
    CharmsBar.SetFont("s" FS_Big " c" TextColor, "Segoe UI Light")
    TimeCtl := CharmsBar.Add("Text", "x0 y" BottomY " w" CurrentBarWidth " Center BackgroundTrans", FormatTime(, "h:mm"))
    CharmsBar.SetFont("s" FS_Sml " c" TextColor, "Segoe UI")
    AmPmCtl := CharmsBar.Add("Text", "x0 y+0 w" CurrentBarWidth " Center BackgroundTrans", FormatTime(, "tt"))
    CharmsBar.SetFont("s" FS_Sml " c" SubText, "Segoe UI")
    CharmsBar.Add("Text", "x0 y+5 w" CurrentBarWidth " Center BackgroundTrans", FormatTime(, "M/d/yyyy"))

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
    ConfigGui := Gui(, "Charms Settings")
    ConfigGui.BackColor := "202020"
    ConfigGui.SetFont("s10 cWhite", "Segoe UI")
    
    ; DWMWA_USE_IMMERSIVE_DARK_MODE (20)
    DllCall("dwmapi\DwmSetWindowAttribute", "Ptr", ConfigGui.Hwnd, "Int", 20, "Int*", 1, "Int", 4)

    ; Sidebar Background
    ConfigGui.Add("Text", "x0 y0 w140 h450 Background151515")
    
    ; Navigation
    NavBtns := Map()
    Pages := Map()
    CurrentPage := "General"
    
    SwitchPage(Name) {
        For PName, Controls in Pages
            For Ctrl in Controls {
                if !IsObject(Ctrl)
                    continue
                try Ctrl.Visible := (PName = Name)
            }
        
        For BtnName, Btn in NavBtns {
            if !IsObject(Btn)
                continue
            Btn.Opt((BtnName=Name) ? "Background353535" : "Background151515")
            Btn.SetFont((BtnName=Name) ? "w700" : "w400")
        }
    }

    AddNav(Label, Y) {
        Btn := ConfigGui.Add("Text", "x0 y" Y " w140 h40 Center +0x200 BackgroundTrans cWhite", Label)
        Btn.OnEvent("Click", (*) => SwitchPage(Label))
        NavBtns[Label] := Btn
    }
    
    AddNav("General", 50)
    AddNav("Appearance", 95)
    AddNav("Apps", 140)
    AddNav("About", 185)

    ; --- GENERAL PAGE ---
    Pages["General"] := []
    C := Pages["General"]
    C.Push(ConfigGui.Add("GroupBox", "x160 y20 w300 h80 cWhite", "Startup"))
    C.Push(ConfigGui.Add("Checkbox", "xp+15 yp+25 vCbBoot Checked" LaunchOnBoot, "Run at Startup (No UAC)"))
    
    C.Push(ConfigGui.Add("GroupBox", "x160 y+20 w300 h100 cWhite", "Search Provider"))
    RadWin := ConfigGui.Add("Radio", "xp+15 yp+25 Checked" (SearchProvider="Windows"), "Windows Search")
    RadEvt := ConfigGui.Add("Radio", "x+20 yp Checked" (SearchProvider="Everything"), "Everything Search")
    C.Push(RadWin), C.Push(RadEvt)
    C.Push(ConfigGui.Add("Text", "x175 y+10 c80D0FF", "Path:"))
    EdtEvt := ConfigGui.Add("Edit", "x+5 yp-3 w180 Background404040 cWhite", EverythingPath)
    BtnEvt := ConfigGui.Add("Text", "x+5 yp w25 h20 Center +0x200 Background404040 cWhite", "...")
    BtnEvt.OnEvent("Click", (*) => (Sel := FileSelect(3,, "Select Everything.exe", "Executables (*.exe)"), (Sel && EdtEvt.Value := Sel)))
    C.Push(EdtEvt), C.Push(BtnEvt)

    ; --- APPEARANCE PAGE ---
    Pages["Appearance"] := []
    C := Pages["Appearance"]
    C.Push(ConfigGui.Add("Text", "x160 y30 cWhite", "Background Mode:"))
    DDLMode := ConfigGui.Add("DropDownList", "x+10 yp-3 w150 vDDLMode Choose" (BackgroundMode="Dark"?1:BackgroundMode="Light"?2:BackgroundMode="Custom"?3:BackgroundMode="Image"?4:5), ["Dark","Light","Custom","Image","Gradient"])
    C.Push(DDLMode)
    
    C.Push(ConfigGui.Add("Text", "x160 y+15 cWhite", "Text Color:"))
    DDLText := ConfigGui.Add("DropDownList", "x+10 yp-3 w150 Choose" (ForceText="Auto"?1:ForceText="White"?2:3), ["Auto","White","Black"])
    C.Push(DDLText)

    C.Push(ConfigGui.Add("Text", "x160 y+15 cWhite", "Volume Control:"))
    DDLVol := ConfigGui.Add("DropDownList", "x+10 yp-3 w150 Choose" (VolumeStyle="Slider"?2:1), ["Buttons","Slider"])
    C.Push(DDLVol)

    C.Push(ConfigGui.Add("GroupBox", "x160 y+20 w300 h140 cWhite", "Details"))
    C.Push(ConfigGui.Add("Text", "xp+15 yp+25 c80D0FF", "Solid Hex:"))
    E_Hex := ConfigGui.Add("Edit", "x+10 yp-3 w80 Background404040 cWhite", CustomColor)
    C.Push(E_Hex)
    
    C.Push(ConfigGui.Add("Text", "x175 y+15 c80D0FF", "Gradient:"))
    E_Grad := ConfigGui.Add("Edit", "x+10 yp-3 w140 Background404040 cWhite", GradientStr)
    C.Push(E_Grad)
    
    C.Push(ConfigGui.Add("Text", "x175 y+15 c80D0FF", "Image Path:"))
    E_Img := ConfigGui.Add("Edit", "x+10 yp-3 w140 Background404040 cWhite", BgImage)
    BtnImg := ConfigGui.Add("Text", "x+5 yp w25 h20 Center +0x200 Background404040 cWhite", "...")
    BtnImg.OnEvent("Click", (*) => (Sel := FileSelect(3,, "Select Image", "Images (*.jpg; *.png; *.bmp)"), (Sel && E_Img.Value := Sel)))
    C.Push(E_Img), C.Push(BtnImg)

    ; --- APPS PAGE ---
    Pages["Apps"] := []
    C := Pages["Apps"]
    C.Push(ConfigGui.Add("Text", "x160 y30 w300 Center c80D0FF", "Configure Quick Launch Apps"))
    
    AddAppRowUI(Label, NameVar, PathVar, YPos) {
        C.Push(ConfigGui.Add("Text", "x160 y" YPos " w20 cWhite", Label))
        E_Name := ConfigGui.Add("Edit", "x+5 yp-3 w70 Background404040 cWhite", NameVar)
        E_Path := ConfigGui.Add("Edit", "x+5 yp w150 Background404040 cWhite", PathVar)
        Btn := ConfigGui.Add("Text", "x+5 yp w25 h20 Center +0x200 Background404040 cWhite", "...")
        Btn.OnEvent("Click", (*) => (Sel := FileSelect(3,, "Select App"), (Sel && E_Path.Value := Sel)))
        C.Push(E_Name), C.Push(E_Path), C.Push(Btn)
        return [E_Name, E_Path]
    }

    CtrlsA := AddAppRowUI("A:", AppA_Name, AppA_Path, 70)
    CtrlsB := AddAppRowUI("B:", AppB_Name, AppB_Path, 110)
    CtrlsC := AddAppRowUI("C:", AppC_Name, AppC_Path, 150)

    ; --- ABOUT PAGE ---
    Pages["About"] := []
    C := Pages["About"]
    ConfigGui.SetFont("s16 bold")
    C.Push(ConfigGui.Add("Text", "x160 y50 w300 Center cWhite", "Charms Bar Platinum"))
    ConfigGui.SetFont("s11 norm")
    C.Push(ConfigGui.Add("Text", "x160 y+5 w300 Center c80D0FF", "Version " CurrentVersion))
    C.Push(ConfigGui.Add("Text", "x160 y+30 w300 Center cWhite", "Created by Imran Ahmed"))
    ConfigGui.SetFont("s10 underline c80C0FF")
    LinkA := ConfigGui.Add("Text", "x160 y+5 w300 Center", "github.com/omnitx")
    LinkA.OnEvent("Click", (*) => Run("https://github.com/omnitx"))
    C.Push(LinkA)
    ConfigGui.SetFont("s10 norm cWhite")
    C.Push(ConfigGui.Add("Text", "x160 y+20 w300 Center", "Co-Developed with Antigravity"))
    
    BtnUpdate := ConfigGui.Add("Button", "x230 y+10 w160 h30", "Check for Updates")
    BtnUpdate.OnEvent("Click", (*) => CheckForUpdates(true))
    C.Push(BtnUpdate)
    
    ConfigGui.SetFont("s8 cGray")
    C.Push(ConfigGui.Add("Text", "x160 y+10 w300 Center", "(Google DeepMind)"))
    
    ; --- SAVE BUTTON ---
    BtnSave := ConfigGui.Add("Text", "x230 y300 w160 h35 Center +0x200 Background404040 cWhite", "Save & Apply")
    BtnSave.OnEvent("Click", SaveAll)
    
    SwitchPage("General")
    ConfigGui.Show()
    
    SaveAll(*) {
        Global BackgroundMode := DDLMode.Text
        Global CustomColor := StrReplace(E_Hex.Value, "#", "")
        Global BgImage := E_Img.Value
        Global GradientStr := E_Grad.Value
        Global ForceText := DDLText.Text
        Global VolumeStyle := DDLVol.Text
        Global LaunchOnBoot := ConfigGui["CbBoot"].Value
        
        Global SearchProvider := RadWin.Value ? "Windows" : "Everything"
        Global EverythingPath := EdtEvt.Value
        
        Global AppA_Name := CtrlsA[1].Value, AppA_Path := CtrlsA[2].Value
        Global AppB_Name := CtrlsB[1].Value, AppB_Path := CtrlsB[2].Value
        Global AppC_Name := CtrlsC[1].Value, AppC_Path := CtrlsC[2].Value

        SetStartup(LaunchOnBoot)

        IniWrite(LaunchOnBoot, IniFile, "Settings", "LaunchOnBoot")
        IniWrite(VolumeStyle, IniFile, "Settings", "VolumeStyle")
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
    try {
        if (!IsSet(CharmsBar) || !CharmsBar || hwnd != CharmsBar.Hwnd)
            return
    }

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
    IconCtl := CharmsBar.Add("Text", "x0 Center w" CurrentBarWidth " h50 +0x200 BackgroundTrans c" TextColor, Chr(IconCode))
    IconCtl.OnEvent("Click", Callback), IconCtl.ToolTipText := TipText
    CharmsBar.SetFont("s" FontSize " c" TextColor, "Segoe UI")
    TextCtl := CharmsBar.Add("Text", "x0 Center w" CurrentBarWidth " y+0 BackgroundTrans c" TextColor, LabelText)
    TextCtl.OnEvent("Click", Callback), TextCtl.ToolTipText := TipText
    CharmsBar.SetFont("s24", "Segoe MDL2 Assets") 
    CharmsBar.Add("Text", "x0 h10 BackgroundTrans w" CurrentBarWidth)
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
        RunWait('schtasks /Create /TN "' TaskName '" /TR "\"' A_AhkPath '\" \"' A_ScriptFullPath '\"" /SC ONLOGON /RL HIGHEST /F',, "Hide")
    else
        RunWait('schtasks /Delete /TN "' TaskName '" /F',, "Hide")
}

GetImageSize(Path, &W, &H) {
    try {
        hBM := LoadPicture(Path)
        bm := Buffer(32, 0)
        DllCall("GetObject", "Ptr", hBM, "Int", 32, "Ptr", bm.Ptr)
        W := NumGet(bm, 4, "Int")
        H := NumGet(bm, 8, "Int")
        DllCall("DeleteObject", "Ptr", hBM)
    } catch {
        W := 0, H := 0
    }
}

CenterActiveWindow() {
    hwnd := WinExist("A")
    if (!hwnd)
        return
    
    GetMonitorUnderMouse(&ML, &MT, &MR, &MB)
    WinGetPos(&WX, &WY, &WW, &WH, "ahk_id " hwnd)
    
    NewX := ML + (MR - ML - WW) / 2
    NewY := MT + (MB - MT - WH) / 2
    
    WinMove(NewX, NewY,,, "ahk_id " hwnd)
}



ExitFunc(ExitReason, ExitCode) {
    Global ConfigGui, CharmsBar
    OnMessage(0x0200, CheckTooltips, 0) ; Unregister
    SetTimer(CheckFocus, 0)
    SetTimer(UpdateClock, 0)
    SetTimer(RainbowCycle, 0)
    SetTimer(HotCornerWatch, 0)
    
    if IsSet(ConfigGui) && ConfigGui
        ConfigGui.Destroy()
    if IsSet(CharmsBar) && CharmsBar
        CharmsBar.Destroy()
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
    try {
        if BtnA && BtnB && BtnC {
            BtnA.Opt("c" HSVtoRGB(Hue,1,1))
            BtnB.Opt("c" HSVtoRGB(Mod(Hue+60,360),1,1))
            BtnC.Opt("c" HSVtoRGB(Mod(Hue+120,360),1,1))
        }
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

SetVolumeByClick(Ctrl, Info) {
    Global VolBg, CharmsBar
    try {
        CoordMode("Mouse", "Screen")
        MouseGetPos(&MX, &MY)
        CharmsBar.GetPos(&GuiX, &GuiY)
        Ctrl.GetPos(&CtrlX, &CtrlY, &CtrlW, &CtrlH)
        
        ; Calculate click position relative to control
        AbsCtrlX := GuiX + CtrlX
        ClickX := Max(0, Min(CtrlW, MX - AbsCtrlX))
        
        ; Set volume based on click position
        NewVol := Round((ClickX / CtrlW) * 100)
        SoundSetVolume(Max(0, Min(100, NewVol)))
        UpdateVolumeLabel()
    }
}

UpdateVolumeLabel(Ctrl:="", *) {
    Global VolTextCtl, VolIcon, VolBg
    try {
        if (IsObject(Ctrl) && Ctrl.Type = "Slider")
            SoundSetVolume(Ctrl.Value)
            
        Vol := Round(SoundGetVolume())
        
        ; Update text label
        if (IsSet(VolTextCtl) && VolTextCtl)
            VolTextCtl.Text := Vol "%"
        
        ; Update progress bar
        if (IsSet(VolBg) && VolBg)
            VolBg.Value := Vol
            
        ; Update icon based on mute state and volume
        if (IsSet(VolIcon) && VolIcon)
            VolIcon.Text := SoundGetMute() ? Chr(0xE74F) : (Vol=0 ? Chr(0xE74F) : (Vol<30 ? Chr(0xE992) : (Vol<70 ? Chr(0xE993) : Chr(0xE767))))
    }
}

UpdateClock() {
    Global TimeCtl, AmPmCtl
    if (TimeCtl) {
        NewTime := FormatTime(,"h:mm")
        NewAmPm := FormatTime(,"tt")
        if (TimeCtl.Text != NewTime)
            TimeCtl.Text := NewTime
        if (AmPmCtl.Text != NewAmPm)
            AmPmCtl.Text := NewAmPm
    }
}

RunAsUser(Target, Args := "") {
    if GetKeyState("Shift", "P") {
        try Run(Target " " Args)
        return
    }
    if !A_IsAdmin {
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