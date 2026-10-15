#include "AgentPopup.hpp"
#include "Clip.hpp"
#include <Geode/utils/base64.hpp>
#include <Geode/utils/general.hpp>

using namespace geode::prelude;

static constexpr float W = 500.f, H = 320.f;
static constexpr float PREV_X = 285.f, PREV_Y = 140.f, PREV_W = 200.f, PREV_H = 130.f;

AgentPopup* AgentPopup::create(EditorUI* ui) {
    auto ret = new AgentPopup();
    if (ret->init(ui)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

static ButtonSprite* btnSpr(char const* text, int width, char const* tex, float scale = .6f) {
    return ButtonSprite::create(text, width, true, "goldFont.fnt", tex, 28.f, scale);
}

static CCScale9Sprite* panel(float w, float h, float x, float y, GLubyte opacity = 110) {
    auto bg = CCScale9Sprite::create("square02b_001.png", {0, 0, 80, 80});
    bg->setContentSize({w, h});
    bg->setColor({0, 0, 0});
    bg->setOpacity(opacity);
    bg->setAnchorPoint({0, 0});
    bg->setPosition({x, y});
    return bg;
}

bool AgentPopup::init(EditorUI* ui) {
    if (!Popup::init(W, H)) return false;
    m_ui = ui;
    m_res = (int)Mod::get()->getSettingValue<int64_t>("resolution");

    // ---- header -------------------------------------------------------
    auto grad = CCLayerGradient::create({255, 70, 150, 255}, {110, 80, 255, 255}, {1, 0});
    grad->setContentSize({W - 24.f, 30.f});
    grad->setPosition({12.f, H - 44.f});
    m_mainLayer->addChild(grad, 0);

    auto title = CCLabelBMFont::create("R387", "bigFont.fnt");
    title->setScale(.6f);
    title->setAnchorPoint({0, .5f});
    title->setPosition({22.f, H - 29.f});
    m_mainLayer->addChild(title, 1);

    auto sub = CCLabelBMFont::create("Art Agent", "goldFont.fnt");
    sub->setScale(.5f);
    sub->setAnchorPoint({0, .5f});
    sub->setPosition({95.f, H - 30.f});
    m_mainLayer->addChild(sub, 1);

    m_balance = CCLabelBMFont::create("Generations: ...", "bigFont.fnt");
    m_balance->setScale(.38f);
    m_balance->setAnchorPoint({1, .5f});
    m_balance->setPosition({W - 22.f, H - 29.f});
    m_mainLayer->addChild(m_balance, 1);

    // ---- left column --------------------------------------------------
    m_prompt = TextInput::create(200.f, "Describe the art (any language)", "chatFont.fnt");
    m_prompt->setCommonFilter(CommonFilter::Any);
    m_prompt->setMaxCharCount(400);
    m_prompt->setPosition({115.f, H - 75.f});
    m_mainLayer->addChild(m_prompt);

    auto attach = CCMenuItemSpriteExtra::create(btnSpr("Attach", 56, "GJ_button_04.png", .5f), this,
                                                menu_selector(AgentPopup::onAttach));
    attach->setPosition({251.f, H - 75.f});
    m_buttonMenu->addChild(attach);

    m_refLabel = CCLabelBMFont::create("No reference picture (Ctrl+V pastes one)", "chatFont.fnt");
    m_refLabel->setScale(.45f);
    m_refLabel->setAnchorPoint({0, .5f});
    m_refLabel->setPosition({16.f, H - 100.f});
    m_refLabel->setOpacity(190);
    m_mainLayer->addChild(m_refLabel);

    m_shape = CCLabelBMFont::create("", "chatFont.fnt");
    m_shape->setScale(.5f);
    m_shape->setAnchorPoint({0, .5f});
    m_shape->setPosition({16.f, H - 116.f});
    m_mainLayer->addChild(m_shape);

    auto gen = CCMenuItemSpriteExtra::create(btnSpr("Generate", 110, "GJ_button_01.png"), this,
                                             menu_selector(AgentPopup::onGenerate));
    gen->setPosition({71.f, 175.f});
    m_buttonMenu->addChild(gen);

    auto var = CCMenuItemSpriteExtra::create(btnSpr("Another variant", 130, "GJ_button_02.png", .5f), this,
                                             menu_selector(AgentPopup::onVariant));
    var->setPosition({201.f, 175.f});
    m_buttonMenu->addChild(var);

    auto undo = CCMenuItemSpriteExtra::create(btnSpr("Undo", 50, "GJ_button_06.png", .45f), this,
                                              menu_selector(AgentPopup::onUndo));
    undo->setPosition({248.f, 148.f});
    m_buttonMenu->addChild(undo);

    m_spinner = CCSprite::create("loadingCircle.png");
    m_spinner->setScale(.3f);
    m_spinner->setPosition({26.f, 148.f});
    m_spinner->setVisible(false);
    m_spinner->runAction(CCRepeatForever::create(CCRotateBy::create(1.f, 360.f)));
    m_mainLayer->addChild(m_spinner);

    m_status = CCLabelBMFont::create("", "chatFont.fnt");
    m_status->setScale(.5f);
    m_status->setAnchorPoint({0, .5f});
    m_status->setPosition({42.f, 148.f});
    m_mainLayer->addChild(m_status);

    // ---- preview ------------------------------------------------------
    m_mainLayer->addChild(panel(PREV_W, PREV_H, PREV_X, PREV_Y, 140), 0);
    m_preview = CCDrawNode::create();
    m_preview->setPosition({PREV_X, PREV_Y});
    m_mainLayer->addChild(m_preview, 1);

    // ---- chat ---------------------------------------------------------
    m_mainLayer->addChild(panel(W - 30.f, 72.f, 15.f, 58.f, 110), 0);
    m_chatInput = TextInput::create(360.f, "Ask for changes: make the sky purple...", "chatFont.fnt");
    m_chatInput->setCommonFilter(CommonFilter::Any);
    m_chatInput->setMaxCharCount(300);
    m_chatInput->setPosition({195.f, 35.f});
    m_mainLayer->addChild(m_chatInput);

    auto send = CCMenuItemSpriteExtra::create(btnSpr("Send", 70, "GJ_button_03.png", .55f), this,
                                              menu_selector(AgentPopup::onSend));
    send->setPosition({420.f, 35.f});
    m_buttonMenu->addChild(send);

    // ---- selection ----------------------------------------------------
    m_mask = r387::maskFrom(m_ui->getSelectedObjects());
    if (!m_mask.err.empty()) {
        m_shape->setString(m_mask.err.c_str());
        m_shape->setColor({255, 140, 140});
        chat("R387", "Select the blocks you want painted, then describe the art.");
    } else {
        m_shape->setString(fmt::format("Shape: {}x{} blocks, {} cells", m_mask.w, m_mask.h, m_mask.cells).c_str());
        m_shape->setColor({150, 255, 190});
        chat("R387", "Shape locked in. Tell me what to draw in it.");
    }
    drawPreview();
    fetchBalance();
    this->setKeyboardEnabled(true);
    return true;
}

std::string AgentPopup::serverUrl() const {
    auto u = Mod::get()->getSettingValue<std::string>("server-url");
    while (!u.empty() && u.back() == '/') u.pop_back();
    return u;
}

// ---- chat -------------------------------------------------------------
void AgentPopup::chat(std::string const& who, std::string const& text) {
    auto t = text.size() > 150 ? text.substr(0, 147) + "..." : text;
    m_log.push_back(who + ": " + t);
    refreshChat();
}

void AgentPopup::refreshChat() {
    if (m_chatArea) m_chatArea->removeFromParent();
    std::string all;
    size_t start = m_log.size() > 4 ? m_log.size() - 4 : 0;
    for (size_t i = start; i < m_log.size(); i++) all += m_log[i] + "\n";
    m_chatArea = SimpleTextArea::create(all, "chatFont.fnt", .5f, W - 50.f);
    m_chatArea->setAnchorPoint({0, 1});
    m_chatArea->setPosition({22.f, 126.f});
    m_mainLayer->addChild(m_chatArea, 2);
}

void AgentPopup::setBusy(bool busy, std::string const& text) {
    m_busy = busy;
    m_spinner->setVisible(busy);
    m_status->setString(text.c_str());
}

// ---- reference picture -----------------------------------------------
void AgentPopup::setReference(ByteVector const& bytes, std::string const& mime, std::string const& name) {
    if (bytes.empty() || bytes.size() > 12 * 1024 * 1024) {
        chat("R387", "That picture is empty or too large (max 12 MB).");
        return;
    }
    m_refB64 = utils::base64::encode(bytes);
    m_refMime = mime;
    m_refLabel->setString(("Reference: " + name).c_str());
    m_refLabel->setColor({150, 255, 190});
    chat("R387", "Reference picture attached. I will use it as inspiration.");
}

void AgentPopup::onAttach(CCObject*) {
    file::FilePickOptions opts;
    opts.filters.push_back({"Pictures", {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.webp", "*.gif"}});
    m_pickHolder.spawn(
        file::pick(file::PickMode::OpenFile, opts),
        [this](Result<std::optional<std::filesystem::path>> result) {
            if (!result.isOk()) return;
            auto opt = result.unwrap();
            if (!opt) return; // cancelled
            auto path = *opt;
            auto data = file::readBinary(path);
            if (!data) { chat("R387", "Could not read that file."); return; }
            auto ext = utils::string::toLower(path.extension().string());
            std::string mime = ext == ".png" ? "image/png" : ext == ".bmp" ? "image/bmp" :
                               ext == ".webp" ? "image/webp" : ext == ".gif" ? "image/gif" : "image/jpeg";
            setReference(data.unwrap(), mime, path.filename().string());
        });
}

void AgentPopup::pasteClipboard() {
    if (auto img = r387::clipboardImage()) {
        setReference(*img, "image/bmp", "pasted picture");
        return;
    }
    auto text = clipboard::read();
    if (!text.empty()) {
        // Pasting is also the reliable way to enter non-Latin text.
        auto& target = (m_chatInput && !m_art.rows.empty()) ? m_chatInput : m_prompt;
        target->setString(std::string(target->getString()) + text);
    }
}

void AgentPopup::keyDown(enumKeyCodes key) {
    auto kd = CCKeyboardDispatcher::get();
    if (key == KEY_V && (kd->getControlKeyPressed() || kd->getCommandKeyPressed())) {
        pasteClipboard();
        return;
    }
    Popup::keyDown(key);
}

// ---- actions ----------------------------------------------------------
void AgentPopup::onGenerate(CCObject*) {
    if (m_busy) return;
    auto p = m_prompt->getString();
    if (p.empty() && m_refB64.empty()) { chat("R387", "Describe what to draw first."); return; }
    m_basePrompt = p;
    m_instruction.clear();
    m_art = {};
    m_seed = 0;
    chat("You", p.empty() ? "(use the reference picture)" : p);
    request();
}

void AgentPopup::onVariant(CCObject*) {
    if (m_busy) return;
    if (m_basePrompt.empty()) m_basePrompt = m_prompt->getString();
    if (m_basePrompt.empty() && m_refB64.empty()) { chat("R387", "Describe what to draw first."); return; }
    m_instruction.clear();
    m_art = {};
    m_seed++;
    chat("You", "Another variant, please.");
    request();
}

void AgentPopup::onSend(CCObject*) {
    if (m_busy) return;
    auto text = m_chatInput->getString();
    if (text.empty()) return;
    m_chatInput->setString("");
    chat("You", text);
    if (m_art.rows.empty()) {
        m_basePrompt = m_basePrompt.empty() ? text : m_basePrompt + ". " + text;
        m_instruction.clear();
    } else {
        m_instruction = text;
    }
    request();
}

void AgentPopup::onUndo(CCObject*) {
    if (m_placed.empty()) { chat("R387", "Nothing to undo from this session."); return; }
    clearPlaced();
    chat("R387", "Removed the generated art.");
}

void AgentPopup::clearPlaced() {
    auto lel = m_ui->m_editorLayer;
    for (auto& obj : m_placed)
        if (obj && lel->m_objects->containsObject(obj)) lel->removeObject(obj, true);
    m_placed.clear();
}

// ---- network ----------------------------------------------------------
void AgentPopup::fetchBalance() {
    auto key = Mod::get()->getSettingValue<std::string>("license-key");
    web::WebRequest req;
    req.param("license", key);
    m_balHolder.spawn(req.get(serverUrl() + "/v1/balance"), [this](web::WebResponse res) {
        auto j = res.json().unwrapOr(matjson::Value());
        auto b = j["balance"].asInt().unwrapOr(-1);
        m_balance->setString(b >= 0 ? fmt::format("Generations: {}", b).c_str() : "Generations: ?");
        m_balance->setColor(b == 0 ? ccColor3B{255, 120, 120} : ccColor3B{255, 255, 255});
    });
}

void AgentPopup::request() {
    m_mask = r387::maskFrom(m_ui->getSelectedObjects());
    if (!m_mask.err.empty() && m_mask.w == 0) { chat("R387", m_mask.err); return; }
    if (!m_mask.err.empty()) { chat("R387", m_mask.err); return; }
    m_res = (int)Mod::get()->getSettingValue<int64_t>("resolution");

    auto rows = r387::expand(m_mask, m_res);
    std::vector<matjson::Value> maskRows;
    for (auto& r : rows) maskRows.push_back(matjson::Value(r));

    matjson::Value body;
    body["license"] = Mod::get()->getSettingValue<std::string>("license-key");
    body["prompt"] = m_basePrompt;
    body["instruction"] = m_instruction;
    body["previous"] = m_art.id;
    body["variant"] = m_seed;
    body["palette"] = (int)Mod::get()->getSettingValue<int64_t>("palette");
    body["w"] = m_mask.w * m_res;
    body["h"] = m_mask.h * m_res;
    body["mask"] = matjson::Value(maskRows);
    if (!m_refB64.empty()) {
        body["reference"] = m_refB64;
        body["reference_mime"] = m_refMime;
    }

    web::WebRequest req;
    req.header("Content-Type", "application/json");
    req.timeout(std::chrono::seconds(180));
    req.bodyJSON(body);
    m_genHolder.spawn(req.post(serverUrl() + "/v1/generate"), [this](web::WebResponse res) {
        onResponse(res);
    });
    setBusy(true, "R387 is painting...");
}

void AgentPopup::onResponse(web::WebResponse& res) {
    setBusy(false);
    auto j = res.json().unwrapOr(matjson::Value());
    if (!res.ok() || j.contains("error")) {
        if (res.code() == 402) {
            chat("R387", "You are out of generations. Get a pack and put your key in the mod settings.");
            m_balance->setString("Generations: 0");
            m_balance->setColor({255, 120, 120});
        } else if (res.code() == 0) {
            chat("R387", "Cannot reach the server. Check the Server URL in settings.");
        } else {
            chat("R387", j["error"].asString().unwrapOr("Something went wrong (" + std::to_string(res.code()) + ")."));
        }
        return;
    }

    r387::Art art;
    art.id = j["id"].asString().unwrapOr("");
    for (auto& c : j["palette"].asArray().unwrapOrDefault()) {
        auto hex = c.asString().unwrapOr("#ffffff");
        unsigned r = 255, g = 255, b = 255;
        sscanf(hex.c_str() + (hex[0] == '#' ? 1 : 0), "%02x%02x%02x", &r, &g, &b);
        art.palette.push_back({(GLubyte)r, (GLubyte)g, (GLubyte)b});
    }
    for (auto& r : j["rows"].asArray().unwrapOrDefault()) art.rows.push_back(r.asString().unwrapOr(""));
    if (art.palette.empty() || art.rows.empty()) { chat("R387", "The server sent back empty art. Try again."); return; }
    m_art = std::move(art);

    auto bal = j["balance"].asInt().unwrapOr(-1);
    if (bal >= 0) m_balance->setString(fmt::format("Generations: {}", bal).c_str());
    drawPreview();
    place();
    auto msg = j["message"].asString().unwrapOr("");
    chat("R387", msg.empty() ? "Done! Ask for changes below, or press Another variant." : msg);
}

// ---- preview & placement ---------------------------------------------
void AgentPopup::drawPreview() {
    m_preview->clear();
    if (m_mask.w == 0) return;
    int pw = m_mask.w * m_res, ph = m_mask.h * m_res;
    float cs = std::min((PREV_W - 12.f) / pw, (PREV_H - 12.f) / ph);
    float ox = (PREV_W - cs * pw) / 2.f, oy = (PREV_H - cs * ph) / 2.f;
    auto ex = r387::expand(m_mask, m_res);
    for (int r = 0; r < ph; r++) {
        int c = 0;
        while (c < pw) {
            if (ex[r][c] != '1') { c++; continue; }
            int idx = -1;
            if (r < (int)m_art.rows.size() && c < (int)m_art.rows[r].size())
                idx = r387::paletteIndex(m_art.rows[r][c], m_art.palette.size());
            int e = c + 1;
            while (e < pw && ex[r][e] == '1') {
                int i2 = -1;
                if (r < (int)m_art.rows.size() && e < (int)m_art.rows[r].size())
                    i2 = r387::paletteIndex(m_art.rows[r][e], m_art.palette.size());
                if (i2 != idx) break;
                e++;
            }
            ccColor4F col = idx < 0 ? ccColor4F{.28f, .28f, .34f, 1.f}
                                    : ccColor4F{m_art.palette[idx].r / 255.f, m_art.palette[idx].g / 255.f,
                                                m_art.palette[idx].b / 255.f, 1.f};
            m_preview->drawRect({ox + c * cs, oy + (ph - r - 1) * cs}, {ox + e * cs + .3f, oy + (ph - r) * cs + .3f},
                                col, 0.f, col);
            c = e;
        }
    }
}

void AgentPopup::place() {
    clearPlaced();
    int count = 0;
    bool truncated = false;
    auto s = r387::buildObjectString(
        m_mask, m_art, m_res,
        (int)Mod::get()->getSettingValue<int64_t>("block-id"),
        (int)Mod::get()->getSettingValue<int64_t>("color-channel-base"),
        (int)Mod::get()->getSettingValue<int64_t>("max-objects"), count, truncated);
    auto arr = m_ui->m_editorLayer->createObjectsFromString(s, true, true);
    if (arr) for (auto obj : CCArrayExt<GameObject*>(arr)) m_placed.push_back(obj);
    if (truncated) chat("R387", "Hit the object limit, part of the art was left out. Lower Detail or raise the limit.");
}
