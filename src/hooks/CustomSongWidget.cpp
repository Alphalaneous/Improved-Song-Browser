#include "CustomSongWidget.hpp"
#include "../SongItem.hpp"

class LevelSearchViewLayer : public CCLayer {};

bool MyCustomSongWidget::init(SongInfoObject* songInfo, CustomSongDelegate* songDelegate, bool showSongSelect, bool showPlayMusic, bool showDownload, bool isRobtopSong, bool unkBool, bool isMusicLibrary, int unk) {
    auto fields = m_fields.self();
    
    auto scene = CCDirector::get()->getRunningScene();
    if (scene->getChildByType<LevelSearchViewLayer>(0)) {
        showSongSelect = true;
    }

    auto moreSearchLayer = scene->getChildByType<MoreSearchLayer>(0);
    if (moreSearchLayer) {
        showSongSelect = true;
        auto songBrowser = scene->getChildByType<GJSongBrowser>(0);
        if (songBrowser) {
            songBrowser->m_songID = utils::numFromString<int>(moreSearchLayer->m_enterSongID->getString()).unwrapOr(0);
        }
    }
    
    if (!CustomSongWidget::init(songInfo, songDelegate, showSongSelect, showPlayMusic, showDownload, isRobtopSong, unkBool, isMusicLibrary, unk)) return false;
    
    if (showSongSelect) {
        fields->m_deleteButonPos = CCPoint{m_deleteBtn->getPositionX() - 36.f, m_deleteBtn->getPositionY() - 5.f};
        m_deleteBtn->setScale(0.75f);
        m_deleteBtn->m_baseScale = 0.75f;
    }

    runAction(CallFuncExt::create([this] {
        songStateChanged();
    }));

    return true;
}

void MyCustomSongWidget::onPlayback(CCObject* sender) {
    CustomSongWidget::onPlayback(sender);
    // fixing advanced song preview not updating play state
    MusicDownloadManager::sharedState()->songStateChanged();
}

void MyCustomSongWidget::onSelect(CCObject* sender) {
    CustomSongWidget::onSelect(sender);
    auto editorUI = EditorUI::get();
    auto scene = CCDirector::get()->getRunningScene();

    if (editorUI) {
        auto songTrigger = scene->getChildByType<SetupSongTriggerPopup>(0);
        auto songLayer = scene->getChildByType<CustomSongLayer>(0);

        if (songTrigger) {
            auto songLayer = scene->getChildByType<CustomSongLayer>(0);
            if (songLayer) {
                songLayer->m_songWidget->updateSongObject(m_songInfoObject);
                MusicDownloadManager::sharedState()->songStateChanged();
            }
        }
        else if (songLayer) {
            songLayer->m_songWidget->updateSongObject(m_songInfoObject);
            editorUI->m_editorLayer->m_levelSettings->m_level->m_songID = m_customSongID;
            MusicDownloadManager::sharedState()->songStateChanged();
        }
    }
    if (scene->getChildByType<MoreSearchLayer>(0) || scene->getChildByType<LevelSearchViewLayer>(0)) {
        m_selectSongBtn->setVisible(true);
        m_deleteBtn->setVisible(true);
        auto songBrowser = scene->getChildByType<GJSongBrowser>(0);
        if (songBrowser) {
            songBrowser->m_selected = true;
            songBrowser->m_songID = m_customSongID;
        }
    }
}

void MyCustomSongWidget::songStateChanged() {
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

    auto editorUI = EditorUI::get();
    if (editorUI) {
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