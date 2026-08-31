#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/GJSongBrowser.hpp>
#include "../SongList.hpp"

using namespace geode::prelude;

class $modify(MyGJSongBrowser, GJSongBrowser) {
    enum class SortMode {
        Priority,
        Name,
        Artist,
        ID
    };

	struct Fields {
		std::vector<SongData> m_songData;
		std::vector<SongData> m_searchSongData;
		Ref<SongList> m_songList;
		std::vector<Ref<CCMenuItemToggler>> m_filterTogglers;
		geode::TextInput* m_searchInput;
		CCNode* m_background;
		CCMenuItemToggler* m_orderToggler;
		SortMode m_currentSortMode = SortMode::Priority;
		bool m_ascending;
		bool m_isSearching;
	};

    void customSetup();
	void handleSearch(ZStringView str);
	void setMode(SortMode mode, CCMenuItemToggler* toggler);
	void setupListOfSongs();
	void updateCountText();
	void onPrev(CCObject* sender);
	void onNext(CCObject* sender);
	void sortMusic(SortMode sortMode, bool ascending);
	CCMenuItemToggler* createToggler(ZStringView spr, ZStringView id, geode::Function<void(CCMenuItemToggler* toggler)> callback, bool alt, float scale, bool single = false);
};