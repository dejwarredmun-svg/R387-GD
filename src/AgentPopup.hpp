#pragma once
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/async.hpp>
#include <optional>
#include "Painter.hpp"

class AgentPopup : public geode::Popup {
protected:
    EditorUI* m_ui = nullptr;
    geode::TextInput* m_prompt = nullptr;
    geode::TextInput* m_chatInput = nullptr;
    cocos2d::CCLabelBMFont* m_balance = nullptr;
    cocos2d::CCLabelBMFont* m_shape = nullptr;
    cocos2d::CCLabelBMFont* m_refLabel = nullptr;
    cocos2d::CCLabelBMFont* m_status = nullptr;
    cocos2d::CCSprite* m_spinner = nullptr;
    cocos2d::CCDrawNode* m_preview = nullptr;
    geode::SimpleTextArea* m_chatArea = nullptr;

    r387::Mask m_mask;
    r387::Art m_art;
    int m_res = 2;
    int m_seed = 0;
    bool m_busy = false;
    std::string m_basePrompt, m_instruction, m_refB64, m_refMime;
    std::vector<std::string> m_log;
    std::vector<geode::Ref<GameObject>> m_placed;

    geode::async::TaskHolder<geode::web::WebResponse> m_genHolder;
    geode::async::TaskHolder<geode::web::WebResponse> m_balHolder;
    geode::async::TaskHolder<geode::Result<std::optional<std::filesystem::path>>> m_pickHolder;

    bool init(EditorUI* ui);
    void keyDown(cocos2d::enumKeyCodes key) override;

    void onGenerate(cocos2d::CCObject*);
    void onVariant(cocos2d::CCObject*);
    void onSend(cocos2d::CCObject*);
    void onAttach(cocos2d::CCObject*);
    void onUndo(cocos2d::CCObject*);

    void request();
    void onResponse(geode::web::WebResponse& res);
    void fetchBalance();
    void setBusy(bool busy, std::string const& text = "");
    void chat(std::string const& who, std::string const& text);
    void refreshChat();
    void drawPreview();
    void place();
    void clearPlaced();
    void setReference(geode::ByteVector const& bytes, std::string const& mime, std::string const& name);
    void pasteClipboard();
    std::string serverUrl() const;

public:
    static AgentPopup* create(EditorUI* ui);
};
