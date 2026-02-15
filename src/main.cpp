#include <Geode/Geode.hpp>
#include <Geode/modify/GJSongBrowser.hpp>
#include <Geode/modify/CustomSongWidget.hpp>
#include <Geode/modify/CCTextInputNode.hpp>
#include "SongList.hpp"
#include "SongItem.hpp"

// thanks mat
#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include <Geode/external/fts/fts_fuzzy_match.h>

using namespace geode::prelude;

enum SortMode {
	PRIORITY,
	NAME,
	ARTIST,
	ID
};

class LevelSearchViewLayer : public CCLayer {};

class $modify(MyCustomSongWidget, CustomSongWidget) {

	struct Fields {
		CCPoint m_deleteButonPos;
	};

    bool init(SongInfoObject* songInfo, CustomSongDelegate* songDelegate, bool showSongSelect, bool showPlayMusic, bool showDownload, bool isRobtopSong, bool unkBool, bool isMusicLibrary, int unk) {
		auto fields = m_fields.self();
		
		auto scene = CCDirector::get()->getRunningScene();
		if (scene->getChildByType<LevelSearchViewLayer>(0)) {
			showSongSelect = true;
		}
		if (auto moreSearchLayer = scene->getChildByType<MoreSearchLayer>(0)) {
			showSongSelect = true;
			if (GJSongBrowser* songBrowser = scene->getChildByType<GJSongBrowser>(0)) {
				songBrowser->m_songID = utils::numFromString<int>(moreSearchLayer->m_enterSongID->getString()).unwrapOr(0);
			}
		}
		
		if (!CustomSongWidget::init(songInfo, songDelegate, showSongSelect, showPlayMusic, showDownload, isRobtopSong, unkBool, isMusicLibrary, unk)) return false;
		
		if (showSongSelect) {
			fields->m_deleteButonPos = CCPoint{m_deleteBtn->getPositionX() - 36, m_deleteBtn->getPositionY() - 5};
			m_deleteBtn->setScale(0.75f);
			m_deleteBtn->m_baseScale = 0.75f;
		}

		runAction(CallFuncExt::create([this] {
			songStateChanged();
		}));

		return true;
	}

    void onPlayback(CCObject* sender) {
		CustomSongWidget::onPlayback(sender);
		// fixing advanced song preview not updating play state
		MusicDownloadManager::sharedState()->songStateChanged();
	}

    void onSelect(CCObject* sender) {
		CustomSongWidget::onSelect(sender);
		if (auto editorUI = EditorUI::get()) {
			auto scene = CCDirector::get()->getRunningScene();
			if (auto songTrigger = scene->getChildByType<SetupSongTriggerPopup>(0)) {
				if (auto songLayer = scene->getChildByType<CustomSongLayer>(0)) {
					songLayer->m_songWidget->updateSongObject(m_songInfoObject);
					MusicDownloadManager::sharedState()->songStateChanged();
				}
			}
			else if (auto songLayer = scene->getChildByType<CustomSongLayer>(0)) {
				songLayer->m_songWidget->updateSongObject(m_songInfoObject);
				editorUI->m_editorLayer->m_levelSettings->m_level->m_songID = m_customSongID;
				MusicDownloadManager::sharedState()->songStateChanged();
			}
		}
		auto scene = CCDirector::get()->getRunningScene();
		if (scene->getChildByType<MoreSearchLayer>(0) || scene->getChildByType<LevelSearchViewLayer>(0)) {
			m_selectSongBtn->setVisible(true);
			m_deleteBtn->setVisible(true);
			if (auto songBrowser = scene->getChildByType<GJSongBrowser>(0)) {
				songBrowser->m_selected = true;
				songBrowser->m_songID = m_customSongID;
			}
		}
	}

    void songStateChanged() {
		CustomSongWidget::songStateChanged();
		auto fields = m_fields.self();
		if (typeinfo_cast<SongItem*>(getParent())) {
			m_deleteBtn->setVisible(true);
			if (m_showSelectSongBtn) {
				m_deleteBtn->setPosition(fields->m_deleteButonPos);
			}
		}
		auto scene = CCDirector::get()->getRunningScene();
		if (scene->getChildByType<MoreSearchLayer>(0) || scene->getChildByType<LevelSearchViewLayer>(0)) {
			m_selectSongBtn->setVisible(true);
		}

		if (auto editorUI = EditorUI::get()) {
			if (!scene->getChildByType<SetupSongTriggerPopup>(0)) {
				if (editorUI->m_editorLayer->m_levelSettings->m_level->m_songID == m_customSongID) {
					m_selectSongBtn->setEnabled(false);
					m_selectSongBtn->setSprite(CCSprite::createWithSpriteFrameName("GJ_selectSongOnBtn_001.png"));
				}
				else {
					m_selectSongBtn->setEnabled(true);
					m_selectSongBtn->setSprite(CCSprite::createWithSpriteFrameName("GJ_selectSongBtn_001.png"));
				}
			}
		}
	}
};

class $modify(MyGJSongBrowser, GJSongBrowser) {

	struct Fields {
		std::vector<SongData> m_songData;
		std::vector<SongData> m_searchSongData;
		Ref<SongList> m_songList;
		Ref<CCArray> m_filterTogglers;
		geode::TextInput* m_searchInput;
		CCNode* m_background;
		CCMenuItemToggler* m_orderToggler;
		ButtonSprite* m_orderTogglerSprOn;
		ButtonSprite* m_orderTogglerSprOff;
		SortMode m_currentSortMode = SortMode::PRIORITY;
		bool m_ascending;
		bool m_isSearching;
	};

    void customSetup() {
		GJSongBrowser::customSetup();
		auto fields = m_fields.self();

		auto songs = MusicDownloadManager::sharedState()->getDownloadedSongs();

		for (auto songObject : CCArrayExt<SongInfoObject*>(songs)) {
			SongData songData = {
				.songInfoObject = songObject,
				.songName = songObject->m_songName,
				.songArtist = songObject->m_artistName,
				.songID = songObject->m_songID,
				.priority = songObject->m_priority
			};
			fields->m_songData.push_back(songData);
		}

		fields->m_filterTogglers = CCArray::create();

		auto sortButtons = CCMenu::create();
		sortButtons->setContentSize({20, 280});
		sortButtons->setID("sort-menu"_spr);

		fields->m_orderToggler = createToggler("GJ_sortIcon_001.png", "order-btn"_spr, menu_selector(MyGJSongBrowser::toggleAscend), false, 1);
		fields->m_orderTogglerSprOn = fields->m_orderToggler->m_onButton->getChildByType<ButtonSprite>(0);
		fields->m_orderTogglerSprOn->setCascadeColorEnabled(true);
		fields->m_orderTogglerSprOn->setCascadeOpacityEnabled(true);
		fields->m_orderTogglerSprOff = fields->m_orderToggler->m_offButton->getChildByType<ButtonSprite>(0);
		fields->m_orderTogglerSprOff->setCascadeColorEnabled(true);
		fields->m_orderTogglerSprOff->setCascadeOpacityEnabled(true);

    	sortButtons->addChild(fields->m_orderToggler);

		auto emptyDivider = CCNode::create();
		emptyDivider->setContentSize({1, 5});
		sortButtons->addChild(emptyDivider);

		auto initialToggler = createToggler("GJ_timeIcon_001.png", "recent-btn"_spr, menu_selector(MyGJSongBrowser::toggleRecent), true, 0.95f);
		initialToggler->toggle(true);

		fields->m_filterTogglers->addObject(initialToggler);
		fields->m_filterTogglers->addObject(createToggler("GJ_noteIcon_001.png", "name-btn"_spr, menu_selector(MyGJSongBrowser::toggleName), true, 0.95f));
		fields->m_filterTogglers->addObject(createToggler("artist_icon.png"_spr, "artist-btn"_spr, menu_selector(MyGJSongBrowser::toggleArtist), true, 0.95f, true));
		fields->m_filterTogglers->addObject(createToggler("id_icon.png"_spr, "id-btn"_spr, menu_selector(MyGJSongBrowser::toggleID), true, 0.95f, true));
    
		for (auto toggler : CCArrayExt<CCMenuItemToggler*>(fields->m_filterTogglers)) {
			sortButtons->addChild(toggler);
		}

		sortButtons->setScale(0.65f);

		auto winSize = CCDirector::get()->getWinSize();
		sortButtons->setPositionX(winSize.width/2 - 208);
		sortButtons->setPositionY(winSize.height/2 - 20);

		auto layout = ColumnLayout::create();
		layout->setAxisAlignment(AxisAlignment::End);
		layout->setAxisReverse(true);
		sortButtons->setLayout(layout);

		sortButtons->updateLayout();

		m_mainLayer->addChild(sortButtons);

		sortMusic(SortMode::PRIORITY, false);
		runAction(CallFuncExt::create([this, fields, winSize] {
			fields->m_background = m_mainLayer->getChildByID("background");

			auto searchBar = CCLayerColor::create({100, 100, 100, 255});
			searchBar->setID("search-bar"_spr);
			searchBar->setContentSize({356, 30});
			searchBar->setPositionY(190);
			float scale = 0.70f;

			fields->m_searchInput = geode::TextInput::create((searchBar->getContentWidth() - 50) / scale, "Search", "bigFont.fnt");
			fields->m_searchInput->setTextAlign(TextInputAlign::Left);
			fields->m_searchInput->setScale(scale);
			fields->m_searchInput->setID("search-input"_spr);
			fields->m_searchInput->setPosition(searchBar->getContentSize()/2);
			fields->m_searchInput->setPositionX(fields->m_searchInput->getPositionX() - 18);
			fields->m_searchInput->getChildByType<CCTextInputNode>(0)->setUserObject("fix"_spr, CCNode::create());
			fields->m_searchInput->setCallback([this] (std::string str) {
				handleSearch(str);
			});
			fields->m_background->addChild(searchBar);
			searchBar->addChild(fields->m_searchInput);

			auto clearSearchBtn = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_longBtn07_001.png"), this, menu_selector(MyGJSongBrowser::clearSearch));
			clearSearchBtn->setPositionX(fields->m_searchInput->getPositionX() + fields->m_searchInput->getScaledContentWidth()/2 + 21);
			clearSearchBtn->setPositionY(fields->m_searchInput->getPositionY());
			clearSearchBtn->setScale(0.7f);
			clearSearchBtn->m_baseScale = 0.7f;
			clearSearchBtn->setID("clear-search-btn"_spr);

			auto clearBtnMenu = CCMenu::create();
			clearBtnMenu->setContentSize(clearSearchBtn->getScaledContentSize());
			clearBtnMenu->setPositionX(fields->m_searchInput->getPositionX() + fields->m_searchInput->getScaledContentWidth()/2 + 21);
			clearBtnMenu->setPositionY(fields->m_searchInput->getPositionY());
			clearBtnMenu->ignoreAnchorPointForPosition(false);
			clearBtnMenu->setID("clear-btn-menu"_spr);

			clearSearchBtn->setPosition(clearBtnMenu->getContentSize()/2);

			clearBtnMenu->addChild(clearSearchBtn);
			searchBar->addChild(clearBtnMenu);

			if (auto listView = fields->m_background->getChildByType<CustomListView>(0)) {
				listView->removeFromParent();
			}
			setupListOfSongs();
		}));

		m_leftArrow->m_pfnSelector = menu_selector(MyGJSongBrowser::onPrev);
		m_rightArrow->m_pfnSelector = menu_selector(MyGJSongBrowser::onNext);
	}

	void handleSearch(ZStringView str) {
		auto fields = m_fields.self();

		fields->m_isSearching = !str.empty();
		fields->m_searchSongData.clear();

		if (!fields->m_isSearching) {
			sortMusic(fields->m_currentSortMode, fields->m_ascending);
			fields->m_orderToggler->setEnabled(true);
			fields->m_orderTogglerSprOn->setColor({255, 255, 255});
			fields->m_orderTogglerSprOn->setOpacity(255);
			fields->m_orderTogglerSprOff->setColor({255, 255, 255});
			fields->m_orderTogglerSprOff->setOpacity(255);
		}
		else {
			fields->m_orderToggler->setEnabled(false);
			fields->m_orderTogglerSprOn->setColor({75, 75, 75});
			fields->m_orderTogglerSprOn->setOpacity(127);
			fields->m_orderTogglerSprOff->setColor({75, 75, 75});
			fields->m_orderTogglerSprOff->setOpacity(127);
		}

		std::vector<std::pair<int, SongData>> songScores;

		for (const auto& data : fields->m_songData) {
			int score = 0;
			std::string sortString;
			switch (fields->m_currentSortMode) {
				case SortMode::PRIORITY:
				case SortMode::NAME:
					sortString = utils::string::toLower(data.songName);
					break;
				case SortMode::ARTIST:
					sortString = utils::string::toLower(data.songArtist);
					break;
				case SortMode::ID:
					sortString = utils::numToString(data.songID);
					break;
			}
			if (!str.empty() && !fts::fuzzy_match(str.c_str(), sortString.c_str(), score)) continue;
			songScores.push_back({score, data});
		}

		std::sort(songScores.begin(), songScores.end(), [&](const auto& a, const auto& b) {
			return a.first > b.first;
		});

		for (const auto& [score, data] : songScores) {
			fields->m_searchSongData.push_back(data);
		}

		m_page = 0;
		setupListOfSongs();
		updateCountText();
	}

	void clearSearch(CCObject* sender) {
		auto fields = m_fields.self();

		fields->m_searchInput->setString("", true);
	}

	void toggleAscend(CCObject* sender) {
		auto fields = m_fields.self();

		fields->m_ascending = !fields->m_ascending;
		sortMusic(fields->m_currentSortMode, fields->m_ascending);
		setupListOfSongs();
	}

	void setMode(SortMode mode, CCObject* sender) {
		auto fields = m_fields.self();

		fields->m_currentSortMode = mode;
		fields->m_searchInput->setCommonFilter(CommonFilter::Any);
		
		if (!fields->m_searchInput->getString().empty()) {
			handleSearch(fields->m_searchInput->getString());
		} else {
			sortMusic(fields->m_currentSortMode, fields->m_ascending);
			setupListOfSongs();
		}

		for (auto toggler : CCArrayExt<CCMenuItemToggler*>(fields->m_filterTogglers)) {
			if (toggler == sender) {
				toggler->setClickable(false);
				toggler->toggle(true);
			}
			else {
				toggler->setClickable(true);
				toggler->toggle(false);
			}
		}
	}

	void toggleRecent(CCObject* sender) {
		setMode(SortMode::PRIORITY, sender);
	}

	void toggleName(CCObject* sender) {
		setMode(SortMode::NAME, sender);
	}

	void toggleArtist(CCObject* sender) {
		setMode(SortMode::ARTIST, sender);
	}

	void toggleID(CCObject* sender) {
		setMode(SortMode::ID, sender);
		auto fields = m_fields.self();

		fields->m_searchInput->setCommonFilter(CommonFilter::Int);
	}

	void setupListOfSongs() {
		auto fields = m_fields.self();

		int rangeStart = m_page * 10;
		int rangeEnd = rangeStart + 10;

		std::vector<SongData> songs;

		if (fields->m_isSearching) {
			songs = fields->m_searchSongData;
		}
		else {
			songs = fields->m_songData;
		}

		if (rangeEnd >= songs.size()) rangeEnd = songs.size();

		std::vector<SongData> data;

		for (int i = rangeStart; i < rangeEnd; i++) {
			data.push_back(songs.at(i));
		}

		if (fields->m_songList) fields->m_songList->removeFromParent();
		fields->m_songList = SongList::create(data);
		fields->m_songList->setID("song-list"_spr);
		handleTouchPriority(this);
		fields->m_background->addChild(fields->m_songList);

		if (songs.size() <= 10) {
			m_leftArrow->setVisible(false);
			m_rightArrow->setVisible(false);
		}
		else {
			m_rightArrow->setVisible(true);
			if (m_page <= 0) {
				m_page = 0;
				m_leftArrow->setVisible(false);
			}
			int maxPages = (songs.size() / 10);
			if (m_page >= maxPages) {
				m_page = maxPages;
				m_rightArrow->setVisible(false);
			}
		}
	}

	void updateCountText() {
		std::vector<SongData> songs;
		auto fields = m_fields.self();

		if (fields->m_isSearching) {
			songs = fields->m_searchSongData;
		}
		else {
			songs = fields->m_songData;
		}

		int pageEnd = m_page * 10 + 10;
		if (pageEnd > songs.size()) pageEnd = songs.size();
		if (songs.size() == 0) {
			m_countText->setString("No Results");
		}
		else {
			m_countText->setString(fmt::format("{} to {} of {}", m_page * 10 + 1, pageEnd, songs.size()).c_str());
		}
	}

	void onPrev(CCObject* sender) {
		m_page--;
		m_rightArrow->setVisible(true);
		if (m_page <= 0) {
			m_page = 0;
			m_leftArrow->setVisible(false);
		}
		updateCountText();
		setupListOfSongs();
	}

	void onNext(CCObject* sender) {
		auto fields = m_fields.self();

		m_page++;
		m_leftArrow->setVisible(true);

		std::vector<SongData> songs;

		if (fields->m_isSearching) {
			songs = fields->m_searchSongData;
		}
		else {
			songs = fields->m_songData;
		}

		int maxPages = (songs.size() / 10);
		if (m_page >= maxPages) {
			m_page = maxPages;
			m_rightArrow->setVisible(false);
		}
		updateCountText();
		setupListOfSongs();
	}

	void sortMusic(SortMode sortMode, bool ascending) {
		auto fields = m_fields.self();

		std::sort(fields->m_songData.begin(), fields->m_songData.end(), [sortMode, ascending] (const SongData& a, const SongData& b) {
			switch (sortMode) {
				case SortMode::PRIORITY:
					return a.priority > b.priority;
				case SortMode::ARTIST:
					return utils::string::toLower(a.songArtist) < utils::string::toLower(b.songArtist);
				case SortMode::NAME:
					return utils::string::toLower(a.songName) < utils::string::toLower(b.songName);
				case SortMode::ID:
					return a.songID < b.songID;
			}
			return true;
		});

		if (ascending) {
			std::reverse(fields->m_songData.begin(), fields->m_songData.end());
		}
	}

	CCMenuItemToggler* createToggler(std::string spr, std::string id, cocos2d::SEL_MenuHandler selector, bool alt, float scale, bool single = false){

		CCSprite* onSpr;
		CCSprite* offSpr;

		if (single) {
			onSpr = CCSprite::create(spr.c_str());
			offSpr = CCSprite::create(spr.c_str());
		}
		else {
			onSpr = CCSprite::createWithSpriteFrameName(spr.c_str());
			offSpr = CCSprite::createWithSpriteFrameName(spr.c_str());
		}

		std::string onSprStr;
		std::string offSprStr;

		if (!alt) {
			onSprStr = "GJ_button_01.png";
			offSprStr = "GJ_button_03.png";
		}
		else {
			onSprStr = "GJ_button_05.png";
			offSprStr = "GJ_button_02.png";
		}

		auto on = ButtonSprite::create(onSpr, 30, true, 30, onSprStr.c_str(), scale);
		auto off = ButtonSprite::create(offSpr, 30, true, 30, offSprStr.c_str(), scale);

		onSpr->setPosition({on->getContentSize().width/2, on->getContentSize().height/2});
		offSpr->setPosition({off->getContentSize().width/2, off->getContentSize().height/2});

		auto toggler = CCMenuItemToggler::create(on, off, this, selector);
		toggler->setID(id);

		return toggler;
	}
};