#include "aiconversation.h"
#include "aimanager.h"

#include <QDebug>

AIConversation::AIConversation(AIManager* aiManager, QObject* parent)
    : QObject(parent)
    , m_aiManager(aiManager)
{
    // Connect to AIManager signals
    if (m_aiManager) {
        connect(m_aiManager, &AIManager::recommendationReceived,
                this, &AIConversation::onAnalysisComplete);
        connect(m_aiManager, &AIManager::errorOccurred,
                this, &AIConversation::onAnalysisFailed);
        connect(m_aiManager, &AIManager::providerChanged,
                this, &AIConversation::providerChanged);
    }
}

QString AIConversation::providerName() const
{
    if (!m_aiManager) return "AI";

    QString provider = m_aiManager->selectedProvider();
    if (provider == "openai") return "GPT";
    if (provider == "anthropic") return "Claude";
    if (provider == "gemini") return "Gemini";
    if (provider == "ollama") return "Ollama";
    return "AI";
}

void AIConversation::ask(const QString& systemPrompt, const QString& userMessage)
{
    if (m_busy || !m_aiManager) return;

    // Clear previous conversation and start fresh
    m_messages = QJsonArray();
    m_systemPrompt = systemPrompt;
    m_lastResponse.clear();
    m_errorMessage.clear();

    addUserMessage(userMessage);
    sendRequest();

    emit historyChanged();
}

void AIConversation::followUp(const QString& userMessage)
{
    if (m_busy || !m_aiManager) return;
    if (m_systemPrompt.isEmpty()) {
        qWarning() << "AIConversation::followUp called without prior ask()";
        return;
    }

    m_errorMessage.clear();
    addUserMessage(userMessage);
    sendRequest();

    emit historyChanged();
}

void AIConversation::clearHistory()
{
    m_messages = QJsonArray();
    m_systemPrompt.clear();
    m_lastResponse.clear();
    m_errorMessage.clear();

    emit historyChanged();
    qDebug() << "AIConversation: History cleared";
}

void AIConversation::addUserMessage(const QString& message)
{
    QJsonObject msg;
    msg["role"] = "user";
    msg["content"] = message;
    m_messages.append(msg);
}

void AIConversation::addAssistantMessage(const QString& message)
{
    QJsonObject msg;
    msg["role"] = "assistant";
    msg["content"] = message;
    m_messages.append(msg);
}

void AIConversation::sendRequest()
{
    if (!m_aiManager || !m_aiManager->isConfigured()) {
        m_errorMessage = "AI not configured";
        emit errorOccurred(m_errorMessage);
        return;
    }

    m_busy = true;
    emit busyChanged();

    // Build the full prompt with conversation history
    // For now, we concatenate messages into a single user prompt
    // since the current AIManager::analyze() doesn't support message arrays
    QString fullPrompt;

    for (int i = 0; i < m_messages.size(); i++) {
        QJsonObject msg = m_messages[i].toObject();
        QString role = msg["role"].toString();
        QString content = msg["content"].toString();

        if (role == "user") {
            if (i > 0) fullPrompt += "\n\n[User follow-up]:\n";
            fullPrompt += content;
        } else if (role == "assistant") {
            fullPrompt += "\n\n[Your previous response]:\n" + content;
        }
    }

    qDebug() << "AIConversation: Sending request with" << m_messages.size() << "messages";
    m_aiManager->analyze(m_systemPrompt, fullPrompt);
}

void AIConversation::onAnalysisComplete(const QString& response)
{
    if (!m_busy) return;  // Not our request

    m_busy = false;
    m_lastResponse = response;

    // Add assistant response to history
    addAssistantMessage(response);

    emit busyChanged();
    emit historyChanged();
    emit responseReceived(response);

    qDebug() << "AIConversation: Response received, history now has" << m_messages.size() << "messages";
}

void AIConversation::onAnalysisFailed(const QString& error)
{
    if (!m_busy) return;  // Not our request

    m_busy = false;
    m_errorMessage = error;

    // Remove the last user message since it failed
    if (!m_messages.isEmpty()) {
        m_messages.removeLast();
    }

    emit busyChanged();
    emit historyChanged();
    emit errorOccurred(error);

    qDebug() << "AIConversation: Request failed:" << error;
}

QString AIConversation::getConversationText() const
{
    QString text;

    for (int i = 0; i < m_messages.size(); i++) {
        QJsonObject msg = m_messages[i].toObject();
        QString role = msg["role"].toString();
        QString content = msg["content"].toString();

        if (i > 0) text += "\n\n---\n\n";

        if (role == "user") {
            text += "You: " + content;
        } else if (role == "assistant") {
            text += providerName() + ": " + content;
        }
    }

    return text;
}
