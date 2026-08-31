#pragma once

#include <Geode/Geode.hpp>
#include "SongList.hpp"

using namespace geode::prelude;

class SongItem : public CCNode {
public:
    static SongItem* create(const SongData&, bool);
protected:
    bool init(const SongData&, bool);
};

