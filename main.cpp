#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include "AgentPopup.hpp"

using namespace geode::prelude;

class $modify(R387EditorUI, EditorUI) {
    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;

        auto label = CCLabelBMFont::create("R387", "goldFont.fnt");
        label->setScale(.55f);
        auto spr = CircleButtonSprite::create(label, CircleBaseColor::Pink, CircleBaseSize::Medium);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(R387EditorUI::onR387));
        btn->setID("r387-button"_spr);

        auto win = CCDirector::get()->getWinSize();
        auto menu = CCMenu::create();
        menu->setID("r387-menu"_spr);
        menu->setPosition({36.f, win.height - 100.f});
        menu->addChild(btn);
        this->addChild(menu, 100);
        return true;
    }

    void onR387(CCObject*) {
        AgentPopup::create(this)->show();
    }
};
