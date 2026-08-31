#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/CustomSongWidget.hpp>

using namespace geode::prelude;

class $modify(MyCustomSongWidget, CustomSongWidget) {

	struct Fields {
		CCPoint m_deleteButonPos;
	};

    bool init(SongInfoObject* songInfo, CustomSongDelegate* songDelegate, bool showSongSelect, bool showPlayMusic, bool showDownload, bool isRobtopSong, bool unkBool, bool isMusicLibrary, int unk);
    void onPlayback(CCObject* sender);
    void onSelect(CCObject* sender);
    void songStateChanged();
};