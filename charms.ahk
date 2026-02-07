; AutoHotkey v2 - Sleek Charms Bar with icon-only buttons
global charmsGui := ""

#c::ToggleCharmsBar()

ToggleCharmsBar() {
    global charmsGui

    if IsObject(charmsGui) {
        charmsGui.Destroy()
        charmsGui := ""
        return
    }

    charmsGui := Gui("AlwaysOnTop -Caption +ToolWindow", "Charms Bar")
    charmsGui.BackColor := "0x252525"
    WinSetTransparent(220, charmsGui.Hwnd)
    charmsGui.SetFont("s20 Bold", "Segoe UI Emoji")

    btnStyle := "w80 h80 +Border -Theme"

    icons := [
        ["🔍", (*) => Run("C:\Program Files\Everything 1.5a\Everything.exe")],
        ["⚙️", (*) => Run("ms-settings:")],
        ["🔇", (*) => Send("{Volume_Mute}")],
        ["⏯️", (*) => Send("{Media_Play_Pause}")],
        ["⏭️", (*) => Send("{Media_Next}")],
        ["⏮️", (*) => Send("{Media_Prev}")],
        ["❌", (*) => (charmsGui.Destroy(), charmsGui := "")]
    ]

    for icon in icons {
        btn := charmsGui.Add("Button", btnStyle, icon[1])
        btn.OnEvent("Click", icon[2])
    }

    screenW := A_ScreenWidth
    screenH := A_ScreenHeight
    barW := screenW * 0.08
    charmsGui.Show("x" (screenW - barW) " y0 w" barW " h" screenH)
}

~LButton:: {
    global charmsGui
    if !IsObject(charmsGui)
        return
    MouseGetPos(&mx, &my)
    charmsGui.GetPos(&gx, &gy, &gw, &gh)
    if (mx < gx || mx > gx+gw || my < gy || my > gy+gh) {
        charmsGui.Destroy()
        charmsGui := ""
    }
}
