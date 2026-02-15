#include "SongList.hpp"
#include "SongItem.hpp"

using namespace geode::prelude;

bool SongList::init(const std::vector<SongData>& songData){

    CCSize contentSize = {356, 190};

    setContentSize(contentSize);

    m_list = ScrollLayer::create(contentSize);
    m_list->m_contentLayer->setLayout(
        ColumnLayout::create()
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
            ->setAutoGrowAxis(contentSize.height)
            ->setGap(0.0f)
    );
    addChildAtPosition(
        m_list,
        Anchor::Center,
        -m_list->getScaledContentSize() / 2
    );

    bool even = false;
    for (const SongData& data : songData) {
        auto item = SongItem::create(data, even);
        m_list->m_contentLayer->addChild(item);
        even = !even;
    }

    m_list->m_contentLayer->updateLayout();
    m_list->scrollToTop();

    return true;
}

SongList* SongList::create(const std::vector<SongData>& songData) {
    auto ret = new SongList();
    if (ret->init(songData)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}