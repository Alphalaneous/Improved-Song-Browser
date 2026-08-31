#include "SongItem.hpp"

bool SongItem::init(const SongData& songData, bool even) {
    setID(fmt::format("song-item-{}", songData.songID));

    setContentSize({356.f, 100.f});
    setAnchorPoint({0.5f, 0.5f});

    auto bgLayer = CCLayerColor::create();
    bgLayer->setContentSize(getContentSize());
    bgLayer->setOpacity(255);
    bgLayer->setID("background-color"_spr);
    bgLayer->setColor(even ? ccColor3B{50, 50, 50} : ccColor3B{75, 75, 75});

    addChild(bgLayer);

    auto scene = CCDirector::get()->getRunningScene();
    auto songLayer = scene->getChildByType<CustomSongLayer>(0);

    auto songWidget = CustomSongWidget::create(songData.songInfoObject, songLayer ? songLayer->m_songDelegate : nullptr, songLayer, true, false, false, false, false, 0);
    songWidget->setID("song-widget");
    songWidget->setPosition(getContentSize() / 2.f);
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