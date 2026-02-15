#include "SongItem.hpp"

bool SongItem::init(const SongData& songData, bool even) {
    if (!CCNode::init()) {
        return false;
    }

    setID(fmt::format("song-item-{}", songData.songID));

    setContentSize({356, 100});
    setAnchorPoint({ 0.5f, 0.5f });

    auto scene = CCDirector::get()->getRunningScene();
    auto bgLayer = CCLayerColor::create();
    bgLayer->setContentSize(getContentSize());
    bgLayer->setOpacity(255);
    bgLayer->setID("background-color"_spr);
    
    if (even) {
        bgLayer->setColor({50, 50, 50});
    }
    else {
        bgLayer->setColor({75, 75, 75});
    }

    addChild(bgLayer);

    CustomSongWidget* songWidget;
    if (auto songLayer = scene->getChildByType<CustomSongLayer>(0)) {
        songWidget = CustomSongWidget::create(songData.songInfoObject, songLayer->m_songDelegate, true, true, false, false, false, false, 0);
    }
    else {
        songWidget = CustomSongWidget::create(songData.songInfoObject, nullptr, false, true, false, false, false, false, 0);
    }
    songWidget->setID("song-widget");
    songWidget->setPosition(getContentSize()/2);
    songWidget->setZOrder(1);
    addChild(songWidget);

    return true;
}

SongItem* SongItem::create(const SongData& songData, bool even) {
    auto ret = new SongItem();
    if (ret->init(songData, even)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}