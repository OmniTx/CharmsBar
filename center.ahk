#Requires AutoHotkey v2.0
; ==============================================================================
; Script: Center Active Window on Monitor Under Mouse (Admin Mode)
; Hotkey: Win + Alt + C
; ==============================================================================

; --- FORCE RUN AS ADMINISTRATOR ---
if not A_IsAdmin
{
    try
    {
        Run "*RunAs `"" A_ScriptFullPath "`""
    }
    catch
    {
        MsgBox("Script requires Admin privileges to move all windows. Please run as Administrator.")
    }
    ExitApp
}

#!c:: ; Hotkey: Win + Alt + C
{
    ; 1. Check if a window is active
    if !WinExist("A")
        return

    ; 2. Get the mouse position relative to the entire screen array
    CoordMode "Mouse", "Screen"
    MouseGetPos &MX, &MY

    ; 3. Determine which monitor the mouse is currently on
    TargetMon := 0
    Loop MonitorGetCount()
    {
        ; Get the bounding box of monitor 'A_Index'
        MonitorGet(A_Index, &ML, &MT, &MR, &MB)
        
        ; Check if mouse coordinates are within these bounds
        if (MX >= ML && MX < MR && MY >= MT && MY < MB)
        {
            TargetMon := A_Index
            break
        }
    }

    ; Fallback: If for some reason mouse isn't found (rare), default to Primary
    if (TargetMon = 0)
        TargetMon := MonitorGetPrimary()

    ; 4. Get the *WorkArea* of the target monitor (excludes taskbar)
    MonitorGetWorkArea(TargetMon, &MonLeft, &MonTop, &MonRight, &MonBottom)

    ; 5. Calculate Monitor Width/Height
    MonW := MonRight - MonLeft
    MonH := MonBottom - MonTop

    ; 6. Get Window Size
    WinGetPos(,, &WinW, &WinH, "A")

    ; 7. Calculate New Position (Center of Target Monitor)
    NewX := MonLeft + (MonW - WinW) / 2
    NewY := MonTop + (MonH - WinH) / 2

    ; 8. Move the window safely
    try 
    {
        ; If the window is maximized, WinMove might not work visually until restored.
        ; However, strictly following "keep size intact", we just move it.
        ; If you want it to un-maximize automatically, uncomment the line below:
        ; WinRestore("A") 
        
        WinMove(NewX, NewY, , , "A")
    }
    catch
    {
        ToolTip "Could not move window: Access Denied"
        SetTimer () => ToolTip(), -2000
    }
}