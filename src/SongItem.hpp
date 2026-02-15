#pragma once

#include <Geode/Geode.hpp>
#include "SongList.hpp"

using namespace geode::prelude;

class SongItem : public CCNode {
protected:
public:
    static SongItem* create(const SongData&, bool);
private:
    bool init(const SongData&, bool);
};

