#pragma once
#include <mkw/menu_assets.hpp>
#include <mkw/menu_animations.hpp>
#include <mkw/menu_movie_assets.hpp>

namespace mkw {
// Hashed, allocation-free views of the generated constexpr menu tables.
// Each returns the FIRST matching row, exactly like the linear searches in
// the generated headers, so composition output is unchanged.
const UiControlPose* uiFindControlPose(const char* control,const char* variant);
const UiControlMotion* uiFindControlMotion(const char* control,const char* variant);
const UiAnimationClip* uiFindAnimationClip(const char* control,const char* group,const char* name);
const UiPanelInfo* uiFindPanel(const char* key);
const UiBorderLoop* uiFindBorderLoop(const char* key);
const UiPaneInfo* uiFindPane(const char* panel,const char* name);
const UiPaneNode* uiFindPaneNode(const char* panel,const char* name);
const UiMovieInfo* uiFindMovie(const char* key);
// Index into UiMovieRegions, or std::size(UiMovieRegions) when absent.
unsigned uiFindMovieRegion(const char* key);
// Index into UiSprites, or std::size(UiSprites) when absent.
unsigned uiFindSpriteKey(const char* key);
}
