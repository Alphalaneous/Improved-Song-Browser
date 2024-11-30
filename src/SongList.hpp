#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

struct SongData {
	SongInfoObject* songInfoObject;
	std::string songName;
	std::string songArtist;
	int songID;
	int priority;
};

class SongList : public CCNode {

    protected:
        ScrollLayer* m_list;
        bool init(std::vector<SongData>);
    public:
        static SongList* create(std::vector<SongData>);
};