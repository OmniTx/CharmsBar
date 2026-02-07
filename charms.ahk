; AutoHotkey v2 - Custom Charms Bar
; Win+C toggles the sidebar
; Buttons mapped to apps/system functions with clear comments

global charmsGui := ""

#c::ToggleCharmsBar()   ; Hotkey: Win+C opens/closes the Charms Bar

ToggleCharmsBar() {
    global charmsGui

    ; If already open → close safely
    if IsObject(charmsGui) {
        SafeClose()
        return
    }

    ; Create GUI window
    charmsGui := Gui("AlwaysOnTop -Caption +ToolWindow", "Charms Bar")
    charmsGui.BackColor := "0x202020"         ; dark background
    WinSetTransparent(220, charmsGui.Hwnd)   ; semi-transparent
    charmsGui.SetFont("s12", "Segoe UI")     ; safe fallback font

    ; Button style (rectangular, dark theme)
    btnStyle := "w180 h40 -Theme"

    ; --- BUTTON MAPPINGS ---
    ; Each entry: [Label, Action]
    buttons := [
        ["Search", (*) => Run("C:\Program Files\Everything 1.5a\Everything.exe")], ; Launch Everything 1.5a
        ["Notepad++", (*) => Run("C:\Program Files\Notepad++\notepad++.exe")],     ; Launch Notepad++
        ["Explorer", (*) => Run("explorer.exe")],                                  ; Open Windows Explorer
        ["Mute", (*) => Send("{Volume_Mute}")],                                    ; Toggle system mute
        ["Play/Pause", (*) => Send("{Media_Play_Pause}")],                         ; Control media playback
        ["Next", (*) => Send("{Media_Next}")],                                     ; Next track
        ["Previous", (*) => Send("{Media_Prev}")]                                  ; Previous track
    ]

    ; Add main buttons
    for b in buttons {
        btn := charmsGui.Add("Button", btnStyle, b[1])
        btn.OnEvent("Click", b[2])
    }

    ; Show docked to right edge with slide-in animation
    screenW := A_ScreenWidth
    screenH := A_ScreenHeight
    barW := screenW * 0.12
    SlideIn(charmsGui, screenW, barW, screenH)
}

; --- SAFE CLOSE FUNCTION ---
SafeClose() {
    global charmsGui
    if IsObject(charmsGui) {
        SlideOut(charmsGui)          ; animate out first
        if IsObject(charmsGui) {
            charmsGui.Destroy()      ; then destroy GUI
            charmsGui := ""
        }
    }
}

; --- SLIDE-IN ANIMATION (fast ease-in-out quadratic) ---
SlideIn(gui, screenW, barW, screenH) {
    steps := 20   ; fewer steps = faster
    Loop steps {
        t := A_Index / steps
        ; Ease-in-out quadratic curve
        if (t < 0.5)
            eased := 2 * (t**2)
        else
            eased := 1 - ((-2*t + 2)**2) / 2

        x := screenW - (barW * eased)
        gui.Show("x" x " y0 w" barW " h" screenH)
        Sleep(5)   ; shorter delay = faster
    }
}

; --- SLIDE-OUT ANIMATION (fast ease-in-out quadratic) ---
SlideOut(gui) {
    if !IsObject(gui)
        return
    gui.GetPos(&gx, &gy, &gw, &gh)
    screenW := A_ScreenWidth
    steps := 20
    Loop steps {
        t := A_Index / steps
        if (t < 0.5)
            eased := 2 * (t**2)
        else
            eased := 1 - ((-2*t + 2)**2) / 2

        x := (screenW - gw) + (gw * eased)
        if IsObject(gui)
            gui.Show("x" x " y0 w" gw " h" gh)
        Sleep(5)
    }
}

; --- OUTSIDE CLICK CLOSE ---
~LButton:: {
    global charmsGui
    if !IsObject(charmsGui)
        return
    MouseGetPos(&mx, &my)
    charmsGui.GetPos(&gx, &gy, &gw, &gh)
    ; If click is outside the Charms Bar → close
    if (mx < gx || mx > gx+gw || my < gy || my > gy+gh) {
        SafeClose()
    }
}
