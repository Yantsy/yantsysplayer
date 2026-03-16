#pragma once
#include "public.h"

class Robot : public QObject {
    Q_OBJECT;
signals:
    void get();

private:
    auto send(std::string& text) {
        // std::string output;
        httplib::Client client("localhost", 8080);
        nlohmann::json body = { { "model", "qwen" },
            { "messages",
                { { { "role", "system" },
                      { "content",
                          "you are a cat girl ,who ends the long sentences with meow,and you "
                          "should "
                          "response in Chinese.But don't just say meow!Just chat with me like a "
                          "good friend." } },
                    { { "role", "user" }, { "content", text } } } },
            { "stream", false } };

        auto ret = client.Post("/v1/chat/completions", body.dump(), "application/json");
        if (!ret) {
            std::cerr << "请求失败，llama-server 是否已启动？" << std::flush;
        }
        if (ret->status != 200) {
            std::cerr << "服务器返回错误: " << ret->status << std::flush;
            std::cerr << ret->body << std::endl;
        }
        auto j = nlohmann::json::parse(ret->body);

        auto& delta = j["choices"][0]["message"];
        if (delta.contains("content")) {
            std::string token = delta["content"];
            words             = QString::fromUtf8(token.c_str(), token.size());
        }
        emit get();
    };

public slots:
    void recv(std::string& text) { send(text); };

public:
    Robot() { };
    ~Robot() { };
    QString words;
};