; AutoHotkey v2 - Charms Bar with rectangular buttons, smooth animation, and Everything Search
global charmsGui := ""

#c::ToggleCharmsBar()   ; Win+C toggles bar

ToggleCharmsBar() {
    global charmsGui

    ; If already open → slide out and close
    if IsObject(charmsGui) {
        SlideOut(charmsGui)
        charmsGui.Destroy()
        charmsGui := ""
        return
    }

    ; Create GUI
    charmsGui := Gui("AlwaysOnTop -Caption +ToolWindow", "Charms Bar")
    charmsGui.BackColor := "0x202020"
    WinSetTransparent(220, charmsGui.Hwnd)
    charmsGui.SetFont("s14", "Magnet")

    ; Rectangular dark buttons
    btnStyle := "Wrap -Theme"

    ; Buttons list
    buttons := [
        ["Search", (*) => Run("C:\Program Files\Everything 1.5a\Everything.exe")], ; adjust path if needed
        ["Settings", (*) => Run("ms-settings:")],
        ["Mute", (*) => Send("{Volume_Mute}")],
        ["Play/Pause", (*) => Send("{Media_Play_Pause}")],
        ["Next", (*) => Send("{Media_Next}")],
        ["Previous", (*) => Send("{Media_Prev}")],
        ["Close", (*) => (SlideOut(charmsGui), charmsGui.Destroy(), charmsGui := "")]
    ]

    for b in buttons {
        btn := charmsGui.Add("Button", btnStyle, b[1])
        btn.OnEvent("Click", b[2])
    }

    ; Show docked to right edge with slide-in
    screenW := A_ScreenWidth
    screenH := A_ScreenHeight
    barW := screenW * 0.12
    SlideIn(charmsGui, screenW, barW, screenH)
}

SlideIn(gui, screenW, barW, screenH) {
    steps := 30
    Loop steps {
        t := A_Index / steps
        ; Ease-in-out quadratic
        if (t < 0.5)
            eased := 2 * (t**2)
        else
            eased := 1 - ((-2*t + 2)**2) / 2

        x := screenW - (barW * eased)
        gui.Show("x" x " y0 w" barW " h" screenH)
        Sleep(10)
    }
}

SlideOut(gui) {
    gui.GetPos(&gx, &gy, &gw, &gh)
    screenW := A_ScreenWidth
    steps := 30
    Loop steps {
        t := A_Index / steps
        ; Ease-in-out quadratic
        if (t < 0.5)
            eased := 2 * (t**2)
        else
            eased := 1 - ((-2*t + 2)**2) / 2

        ; Reverse direction
        x := (screenW - gw) + (gw * eased)
        gui.Show("x" x " y0 w" gw " h" gh)
        Sleep(10)
    }
}


; Close if click outside
~LButton:: {
    global charmsGui
    if !IsObject(charmsGui)
        return
    MouseGetPos(&mx, &my)
    charmsGui.GetPos(&gx, &gy, &gw, &gh)
    if (mx < gx || mx > gx+gw || my < gy || my > gy+gh) {
        SlideOut(charmsGui)
        charmsGui.Destroy()
        charmsGui := ""
    }
}
