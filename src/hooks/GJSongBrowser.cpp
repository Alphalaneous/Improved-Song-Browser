#include "GJSongBrowser.hpp"

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include <Geode/external/fts/fts_fuzzy_match.h>

void MyGJSongBrowser::customSetup() {
    GJSongBrowser::customSetup();
    auto fields = m_fields.self();

    auto songs = MusicDownloadManager::sharedState()->getDownloadedSongs();

    for (auto songObject : songs->asExt<SongInfoObject>()) {
        fields->m_songData.emplace_back(SongData{
            .songInfoObject = songObject,
            .songName = songObject->m_songName,
            .songArtist = songObject->m_artistName,
            .songID = songObject->m_songID,
            .priority = songObject->m_priority
        });
    }

    auto winSize = CCDirector::get()->getWinSize();

    auto sortButtons = CCMenu::create();
    sortButtons->setContentSize({280.f, 20.f});
    sortButtons->setID("sort-menu"_spr);
    sortButtons->setScale(0.5f);
    sortButtons->setAnchorPoint({1.f, 0.5f});
    sortButtons->setPosition(winSize / 2.f - CCPoint{40.f, 122.f});
    sortButtons->setLayout(SimpleRowLayout::create()
        ->setMainAxisAlignment(MainAxisAlignment::Start)
        ->setMainAxisScaling(AxisScaling::Fit)
        ->setCrossAxisScaling(AxisScaling::Fit)
        ->setGap(5.f)
    );

    fields->m_orderToggler = createToggler("GJ_sortIcon_001.png", "order-btn"_spr, [this, fields] (CCMenuItemToggler* toggler) {
        fields->m_ascending = !fields->m_ascending;
        sortMusic(fields->m_currentSortMode, fields->m_ascending);
        setupListOfSongs();
        toggler->toggle(fields->m_ascending);
    }, false, 1.f);

    fields->m_orderToggler->setCascadeColorEnabled(true);
    fields->m_orderToggler->setCascadeOpacityEnabled(true);

    sortButtons->addChild(fields->m_orderToggler);

    auto emptyDivider = CCNode::create();
    emptyDivider->setContentSize({1.f, 5.f});
    sortButtons->addChild(emptyDivider);

    auto initialToggler = createToggler("GJ_timeIcon_001.png", "recent-btn"_spr, [this] (auto toggler) {
        setMode(SortMode::Priority, toggler);
    }, true, 0.95f);

    initialToggler->toggle(true);

    fields->m_filterTogglers.push_back(initialToggler);
    fields->m_filterTogglers.push_back(createToggler("GJ_noteIcon_001.png", "name-btn"_spr, [this] (auto toggler) {
        setMode(SortMode::Name, toggler);
    }, true, 0.95f));

    fields->m_filterTogglers.push_back(createToggler("artist_icon.png"_spr, "artist-btn"_spr, [this] (auto toggler) {
        setMode(SortMode::Artist, toggler);
    }, true, 0.98f, true));

    fields->m_filterTogglers.push_back(createToggler("id_icon.png"_spr, "id-btn"_spr, [this, fields] (auto toggler) {
        setMode(SortMode::ID, toggler);
        fields->m_searchInput->setCommonFilter(CommonFilter::Int);
    }, true, 0.95f, true));

    for (const auto& toggler : fields->m_filterTogglers) {
        sortButtons->addChild(toggler);
    }

    sortButtons->updateLayout();

    m_mainLayer->addChild(sortButtons);

    sortMusic(SortMode::Priority, false);

    runAction(CallFuncExt::create([this, fields, winSize] {
        fields->m_background = m_mainLayer->getChildByID("background");

        auto searchBar = CCLayerColor::create({100, 100, 100, 255});
        searchBar->setID("search-bar"_spr);
        searchBar->setContentSize({356.f, 30.f});
        searchBar->setPositionY(190.f);
        float scale = 0.7f;

        fields->m_searchInput = geode::TextInput::create((searchBar->getContentWidth() - 50.f) / scale, "Search", "bigFont.fnt");
        fields->m_searchInput->setTextAlign(TextInputAlign::Left);
        fields->m_searchInput->setScale(scale);
        fields->m_searchInput->setID("search-input"_spr);
        fields->m_searchInput->setPosition(searchBar->getContentSize()/2);
        fields->m_searchInput->setPositionX(fields->m_searchInput->getPositionX() - 18.f);
        fields->m_searchInput->getChildByType<CCTextInputNode>(0)->setUserObject("fix"_spr, CCNode::create());
        fields->m_searchInput->setCallback([this] (std::string str) {
            handleSearch(str);
        });
        fields->m_background->addChild(searchBar);
        searchBar->addChild(fields->m_searchInput);

        auto clearSearchBtn = CCMenuItemExt::createSpriteExtraWithFrameName("GJ_longBtn07_001.png", 1.f, [this, fields] (auto sender) {
            fields->m_searchInput->setString("", true);
        });

        clearSearchBtn->setPositionX(fields->m_searchInput->getPositionX() + fields->m_searchInput->getScaledContentWidth() / 2.f + 21.f);
        clearSearchBtn->setPositionY(fields->m_searchInput->getPositionY());
        clearSearchBtn->setScale(0.7f);
        clearSearchBtn->m_baseScale = 0.7f;
        clearSearchBtn->setID("clear-search-btn"_spr);

        auto clearBtnMenu = CCMenu::create();
        clearBtnMenu->setContentSize(clearSearchBtn->getScaledContentSize());
        clearBtnMenu->setPositionX(fields->m_searchInput->getPositionX() + fields->m_searchInput->getScaledContentWidth() / 2.f + 21.f);
        clearBtnMenu->setPositionY(fields->m_searchInput->getPositionY());
        clearBtnMenu->ignoreAnchorPointForPosition(false);
        clearBtnMenu->setID("clear-btn-menu"_spr);

        clearSearchBtn->setPosition(clearBtnMenu->getContentSize() / 2.f);

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

void MyGJSongBrowser::handleSearch(ZStringView str) {
    auto fields = m_fields.self();

    fields->m_isSearching = !str.empty();
    fields->m_searchSongData.clear();

    if (!fields->m_isSearching) {
        sortMusic(fields->m_currentSortMode, fields->m_ascending);
        fields->m_orderToggler->setEnabled(true);
        fields->m_orderToggler->setColor({255, 255, 255});
        fields->m_orderToggler->setOpacity(255);
    }
    else {
        fields->m_orderToggler->setEnabled(false);
        fields->m_orderToggler->setColor({75, 75, 75});
        fields->m_orderToggler->setOpacity(127);
    }

    std::vector<std::pair<int, SongData>> songScores;

    for (const auto& data : fields->m_songData) {
        int score = 0;
        std::string sortString;
        switch (fields->m_currentSortMode) {
            case SortMode::Priority:
            case SortMode::Name:
                sortString = utils::string::toLower(data.songName);
                break;
            case SortMode::Artist:
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

void MyGJSongBrowser::setMode(SortMode mode, CCMenuItemToggler* toggler) {
    auto fields = m_fields.self();

    fields->m_currentSortMode = mode;
    fields->m_searchInput->setCommonFilter(CommonFilter::Any);
    
    if (!fields->m_searchInput->getString().empty()) {
        handleSearch(fields->m_searchInput->getString());
    } else {
        sortMusic(fields->m_currentSortMode, fields->m_ascending);
        setupListOfSongs();
    }

    for (const auto& toggler : fields->m_filterTogglers) {
        toggler->toggle(false);
    }
    toggler->toggle(true);
}

void MyGJSongBrowser::setupListOfSongs() {
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

void MyGJSongBrowser::updateCountText() {
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

void MyGJSongBrowser::onPrev(CCObject* sender) {
    m_page--;
    m_rightArrow->setVisible(true);
    if (m_page <= 0) {
        m_page = 0;
        m_leftArrow->setVisible(false);
    }
    updateCountText();
    setupListOfSongs();
}

void MyGJSongBrowser::onNext(CCObject* sender) {
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

void MyGJSongBrowser::sortMusic(SortMode sortMode, bool ascending) {
    auto fields = m_fields.self();

    std::sort(fields->m_songData.begin(), fields->m_songData.end(), [sortMode, ascending] (const SongData& a, const SongData& b) {
        switch (sortMode) {
            case SortMode::Priority:
                return a.priority > b.priority;
            case SortMode::Artist:
                return utils::string::toLower(a.songArtist) < utils::string::toLower(b.songArtist);
            case SortMode::Name:
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

CCMenuItemToggler* MyGJSongBrowser::createToggler(ZStringView spr, ZStringView id, geode::Function<void(CCMenuItemToggler* toggler)> callback, bool alt, float scale, bool single){

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
        onSprStr = "GJ_button_03.png";
        offSprStr = "GJ_button_01.png";
    }
    else {
        onSprStr = "GJ_button_02.png";
        offSprStr = "GJ_button_05.png";
    }

    auto on = ButtonSprite::create(onSpr, 30, true, 30.f, onSprStr.c_str(), scale);
    auto off = ButtonSprite::create(offSpr, 30, true, 30.f, offSprStr.c_str(), scale);

    on->setCascadeOpacityEnabled(true);
    on->setCascadeColorEnabled(true);

    off->setCascadeOpacityEnabled(true);
    off->setCascadeColorEnabled(true);

    onSpr->setPosition(on->getContentSize() / 2.f);
    offSpr->setPosition(off->getContentSize() / 2.f);

    auto toggler = CCMenuItemExt::createToggler(on, off, std::move(callback));
    toggler->setID(id);
    toggler->m_notClickable = true;

    return toggler;
}