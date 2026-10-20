#pragma once

namespace volume {
bool Init();
bool Available();
int  GetVolume();
bool GetMute();
void SetVolume(int v);
void ToggleMute();
}
