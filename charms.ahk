#Requires AutoHotkey v2.0
; ==============================================================================
; Script: Charms Bar (Acrylic Blur + Media Controls)
; Hotkey: Win + C
; Trigger: Hold mouse in Top-Right Corner for 400ms
; ==============================================================================

; Global variables
Global CharmsBar := ""
Global BarWidth := 140  ; Slightly wider to fit media controls
Global OpenedByHover := false
Global CornerHoverStart := 0

; Start the Hot Corner Watcher
SetTimer(HotCornerWatch, 100)

; Define the Hotkey (Win + C)
#c::ToggleCharmsBar(false)

; --- Hot Corner Logic ---
HotCornerWatch() {
    Global OpenedByHover, CharmsBar, CornerHoverStart

    CoordMode "Mouse", "Screen"
    MouseGetPos &MX, &MY
    MonitorGet(MonitorGetPrimary(), &L, &T, &R, &B)
    
    ; Trigger Zone (Top Right Corner, 10x10 pixels)
    InCorner := (MX >= (R - 10) && MY <= (T + 10))

    if (InCorner && !CharmsBar) {
        if (CornerHoverStart == 0) {
            CornerHoverStart := A_TickCount
        }
        else if (A_TickCount - CornerHoverStart > 400) { ; 400ms Delay
            OpenedByHover := true
            ToggleCharmsBar(true)
            CornerHoverStart := 0 
        }
    } else {
        CornerHoverStart := 0
    }

    ; Auto-Close if mouse leaves the bar area
    if (CharmsBar && OpenedByHover) {
        SafeZoneLeft := R - BarWidth
        if (MX < SafeZoneLeft) {
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
            return
        }
        HideCharms()
        return
    }

    CreateCharmsBar()

    ; --- Position the Bar ---
    MonitorGet(MonitorGetPrimary(), &L, &T, &R, &B)
    BarH := B - T
    BarX := R - BarWidth
    BarY := T

    ; Show hidden first
    CharmsBar.Show("x" BarX " y" BarY " w" BarWidth " h" BarH " NoActivate Hide")

    ; --- Apply Acrylic Blur ---
    EnableBlur(CharmsBar.Hwnd)

    ; --- Animate In ---
    DllCall("AnimateWindow", "Ptr", CharmsBar.Hwnd, "Int", 150, "Int", 0x40000 | 0x00002)
    
    WinActivate("ahk_id " CharmsBar.Hwnd)
    SetTimer(CheckFocus, 100)
}

CheckFocus() {
    Global CharmsBar, OpenedByHover
    if (CharmsBar && !WinActive("ahk_id " CharmsBar.Hwnd)) {
        HideCharms()
        OpenedByHover := false
    }
}

HideCharms() {
    Global CharmsBar
    SetTimer(CheckFocus, 0)
    if (!CharmsBar || !WinExist("ahk_id " CharmsBar.Hwnd))
        return

    DllCall("AnimateWindow", "Ptr", CharmsBar.Hwnd, "Int", 150, "Int", 0x10000 | 0x40000 | 0x00001)
    CharmsBar.Destroy()
    CharmsBar := ""
}

RunEverything() {
    targetPath := "C:\Program Files\Everything 1.5a\Everything.exe"
    if FileExist(targetPath) {
        Run targetPath
    } else {
        MsgBox "Could not find Everything at:`n" targetPath
    }
}

; --- Acrylic Blur Function ---
EnableBlur(hwnd) {
    Accent := Buffer(16, 0)
    NumPut("int", 3, Accent, 0) ; 3 = ACCENT_ENABLE_ACRYLICBLURBEHIND
    NumPut("int", 0, Accent, 4) ; GradientColor (0 = transparent)
    
    Data := Buffer(12, 0)
    NumPut("int", 19, Data, 0)  ; 19 = WCA_ACCENT_POLICY
    NumPut("ptr", Accent.Ptr, Data, 4)
    NumPut("int", 16, Data, 8)
    
    DllCall("user32\SetWindowCompositionAttribute", "ptr", hwnd, "ptr", Data)
}

CreateCharmsBar() {
    Global CharmsBar

    ; Create GUI with semi-transparent background for blur
    CharmsBar := Gui("-Caption +AlwaysOnTop +ToolWindow +Owner +E0x08000000", "CharmsBar")
    CharmsBar.BackColor := "111111" ; Dark base
    CharmsBar.MarginX := 0
    CharmsBar.MarginY := 0
    
    ; Make GUI slightly transparent so blur shows through
    WinSetTransparent(220, CharmsBar) 

    CharmsBar.OnEvent("Escape", (*) => HideCharms())
    CharmsBar.SetFont("s26 cWhite", "Segoe MDL2 Assets")

    ; --- Helper: Standard Charm Button ---
    AddCharm(IconCode, LabelText, Callback) {
        IconCtl := CharmsBar.Add("Text", "Center w" BarWidth " h60 +0x200 BackgroundTrans cWhite", Chr(IconCode))
        IconCtl.OnEvent("Click", Callback)
        
        CharmsBar.SetFont("s10 cWhite", "Segoe UI")
        TextCtl := CharmsBar.Add("Text", "Center w" BarWidth " y+0 BackgroundTrans cWhite", LabelText)
        TextCtl.OnEvent("Click", Callback)
        
        CharmsBar.SetFont("s26", "Segoe MDL2 Assets") 
        CharmsBar.Add("Text", "h15 BackgroundTrans w" BarWidth) ; Spacer
    }

    ; --- Top Spacer ---
    CharmsBar.Add("Text", "h50 BackgroundTrans w" BarWidth)

    ; 1. Search
    AddCharm(0xE721, "Search", (*) => (HideCharms(), RunEverything()))
    
    ; 2. Media Controls (Replaces Share)
    ; Header
    CharmsBar.SetFont("s10 cWhite", "Segoe UI")
    CharmsBar.Add("Text", "Center w" BarWidth " BackgroundTrans cWhite", "Media")
    
    ; Controls Row (Prev | Play | Next)
    CharmsBar.SetFont("s16 cWhite", "Segoe MDL2 Assets")
    
    ; We use a 'Progress' trick or simple spacing to align them. 
    ; Here we just add them normally but carefully sized.
    ; Prev (0xE892)
    PrevBtn := CharmsBar.Add("Text", "x15 y+5 w35 h35 Center +0x200 BackgroundTrans cWhite", Chr(0xE892))
    PrevBtn.OnEvent("Click", (*) => Send("{Media_Prev}"))
    
    ; Play/Pause (0xE768)
    PlayBtn := CharmsBar.Add("Text", "x+5 w35 h35 Center +0x200 BackgroundTrans cWhite", Chr(0xE768))
    PlayBtn.OnEvent("Click", (*) => Send("{Media_Play_Pause}"))
    
    ; Next (0xE893)
    NextBtn := CharmsBar.Add("Text", "x+5 w35 h35 Center +0x200 BackgroundTrans cWhite", Chr(0xE893))
    NextBtn.OnEvent("Click", (*) => Send("{Media_Next}"))

    ; Volume Slider
    CharmsBar.SetFont("s10 cWhite", "Segoe UI")
    ; Get current volume
    CurrentVol := SoundGetVolume()
    VolSlider := CharmsBar.Add("Slider", "x10 y+10 w" (BarWidth-20) " h20 ToolTip NoTicks Background111111", CurrentVol)
    ; Update volume when slider moves
    VolSlider.OnEvent("Change", (GuiCtrl, *) => SoundSetVolume(GuiCtrl.Value))
    
    ; Spacer after media
    CharmsBar.Add("Text", "x0 y+20 h15 BackgroundTrans w" BarWidth)
    
    ; Reset Font for big icons
    CharmsBar.SetFont("s26 cWhite", "Segoe MDL2 Assets")

    ; 3. Start (Home Icon 0xE80F)
    AddCharm(0xE80F, "Start", (*) => (HideCharms(), Send("{LWin}")))
    
    ; 4. Devices
    AddCharm(0xE772, "Devices", (*) => (HideCharms(), Send("#k")))
    
    ; 5. Settings
    AddCharm(0xE713, "Settings", (*) => (HideCharms(), Send("#i")))
}