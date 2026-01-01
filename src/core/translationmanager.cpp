#include "translationmanager.h"
#include "settings.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>
#include <QSet>

TranslationManager::TranslationManager(Settings* settings, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_networkManager(new QNetworkAccessManager(this))
{
    // Ensure translations directory exists
    QDir dir(translationsDir());
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Load saved language from settings
    m_currentLanguage = m_settings->value("localization/language", "en").toString();

    // Load language metadata (list of available languages)
    loadLanguageMetadata();

    // Ensure English is always available
    if (!m_languageMetadata.contains("en")) {
        m_languageMetadata["en"] = QVariantMap{
            {"displayName", "English"},
            {"nativeName", "English"},
            {"isRtl", false}
        };
        saveLanguageMetadata();
    }

    // Update available languages list
    m_availableLanguages = m_languageMetadata.keys();
    if (!m_availableLanguages.contains("en")) {
        m_availableLanguages.prepend("en");
    }

    // Load string registry
    loadStringRegistry();

    // Load translations for current language
    loadTranslations();

    // Load AI translations for current language
    loadAiTranslations();

    // Calculate initial untranslated count
    recalculateUntranslatedCount();

    // Timer to batch-save string registry
    QTimer* registrySaveTimer = new QTimer(this);
    registrySaveTimer->setInterval(5000);  // Save every 5 seconds if dirty
    connect(registrySaveTimer, &QTimer::timeout, this, [this]() {
        if (m_registryDirty) {
            saveStringRegistry();
            recalculateUntranslatedCount();
            m_registryDirty = false;
            emit totalStringCountChanged();
        }
    });
    registrySaveTimer->start();

    qDebug() << "TranslationManager initialized. Language:" << m_currentLanguage
             << "Strings:" << m_stringRegistry.size()
             << "Translations:" << m_translations.size()
             << "AI Translations:" << m_aiTranslations.size();
}

QString TranslationManager::translationsDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/translations";
}

QString TranslationManager::languageFilePath(const QString& langCode) const
{
    return translationsDir() + "/" + langCode + ".json";
}

// --- Properties ---

QString TranslationManager::currentLanguage() const
{
    return m_currentLanguage;
}

void TranslationManager::setCurrentLanguage(const QString& lang)
{
    if (m_currentLanguage != lang) {
        m_currentLanguage = lang;
        m_settings->setValue("localization/language", lang);
        loadTranslations();
        loadAiTranslations();
        recalculateUntranslatedCount();
        m_translationVersion++;
        emit translationsChanged();
        emit currentLanguageChanged();
    }
}

bool TranslationManager::editModeEnabled() const
{
    return m_editModeEnabled;
}

void TranslationManager::setEditModeEnabled(bool enabled)
{
    if (m_editModeEnabled != enabled) {
        m_editModeEnabled = enabled;
        emit editModeEnabledChanged();
    }
}

int TranslationManager::untranslatedCount() const
{
    return m_untranslatedCount;
}

int TranslationManager::totalStringCount() const
{
    return m_stringRegistry.size();
}

QStringList TranslationManager::availableLanguages() const
{
    return m_availableLanguages;
}

bool TranslationManager::isDownloading() const
{
    return m_downloading;
}

QString TranslationManager::lastError() const
{
    return m_lastError;
}

// --- Translation lookup ---

QString TranslationManager::translate(const QString& key, const QString& fallback)
{
    // Auto-register the string if not already registered
    if (!m_stringRegistry.contains(key)) {
        m_stringRegistry[key] = fallback;
        // Don't save on every call - batch save periodically
        m_registryDirty = true;
    }

    // Check for custom translation (including English customizations)
    if (m_translations.contains(key) && !m_translations.value(key).isEmpty()) {
        return m_translations.value(key);
    }

    // Return fallback
    return fallback;
}

bool TranslationManager::hasTranslation(const QString& key) const
{
    return m_translations.contains(key) && !m_translations.value(key).isEmpty();
}

// --- Translation editing ---

void TranslationManager::setTranslation(const QString& key, const QString& translation)
{
    m_translations[key] = translation;
    m_aiGenerated.remove(key);  // User edited, no longer AI-generated
    saveTranslations();
    recalculateUntranslatedCount();
    m_translationVersion++;
    emit translationsChanged();
    emit translationChanged(key);
}

void TranslationManager::deleteTranslation(const QString& key)
{
    if (m_translations.contains(key)) {
        m_translations.remove(key);
        saveTranslations();
        recalculateUntranslatedCount();
        m_translationVersion++;
        emit translationsChanged();
        emit translationChanged(key);
    }
}

// --- Language management ---

void TranslationManager::addLanguage(const QString& langCode, const QString& displayName, const QString& nativeName)
{
    if (langCode.isEmpty() || m_languageMetadata.contains(langCode)) {
        return;
    }

    m_languageMetadata[langCode] = QVariantMap{
        {"displayName", displayName},
        {"nativeName", nativeName.isEmpty() ? displayName : nativeName},
        {"isRtl", false}
    };

    saveLanguageMetadata();

    // Create empty translation file
    QFile file(languageFilePath(langCode));
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject root;
        root["language"] = langCode;
        root["displayName"] = displayName;
        root["nativeName"] = nativeName.isEmpty() ? displayName : nativeName;
        root["translations"] = QJsonObject();
        file.write(QJsonDocument(root).toJson());
        file.close();
    }

    m_availableLanguages = m_languageMetadata.keys();
    emit availableLanguagesChanged();

    qDebug() << "Added language:" << langCode << displayName;
}

void TranslationManager::deleteLanguage(const QString& langCode)
{
    if (langCode == "en" || !m_languageMetadata.contains(langCode)) {
        return;  // Can't delete English
    }

    m_languageMetadata.remove(langCode);
    saveLanguageMetadata();

    // Delete translation file
    QFile::remove(languageFilePath(langCode));

    m_availableLanguages = m_languageMetadata.keys();
    emit availableLanguagesChanged();

    // Switch to English if current language was deleted
    if (m_currentLanguage == langCode) {
        setCurrentLanguage("en");
    }

    qDebug() << "Deleted language:" << langCode;
}

QString TranslationManager::getLanguageDisplayName(const QString& langCode) const
{
    if (m_languageMetadata.contains(langCode)) {
        return m_languageMetadata[langCode]["displayName"].toString();
    }
    return langCode;
}

QString TranslationManager::getLanguageNativeName(const QString& langCode) const
{
    if (m_languageMetadata.contains(langCode)) {
        return m_languageMetadata[langCode]["nativeName"].toString();
    }
    return langCode;
}

// --- String registry ---

void TranslationManager::registerString(const QString& key, const QString& fallback)
{
    if (!m_stringRegistry.contains(key)) {
        m_stringRegistry[key] = fallback;
        saveStringRegistry();
        recalculateUntranslatedCount();
        emit totalStringCountChanged();
    }
}

// --- Community translations ---

void TranslationManager::downloadLanguageList()
{
    if (m_downloading) {
        return;
    }

    m_downloading = true;
    emit downloadingChanged();

    QString url = QString("%1/languages.json").arg(GITHUB_RAW_BASE);
    qDebug() << "Fetching language list from:" << url;

    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onLanguageListFetched(reply);
    });
}

void TranslationManager::downloadLanguage(const QString& langCode)
{
    if (m_downloading || langCode == "en") {
        return;
    }

    m_downloading = true;
    m_downloadingLangCode = langCode;
    emit downloadingChanged();

    QString url = QString("%1/languages/%2.json").arg(GITHUB_RAW_BASE, langCode);
    qDebug() << "Fetching language file from:" << url;

    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onLanguageFileFetched(reply);
    });
}

void TranslationManager::onLanguageListFetched(QNetworkReply* reply)
{
    reply->deleteLater();
    m_downloading = false;
    emit downloadingChanged();

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = QString("Failed to fetch language list: %1").arg(reply->errorString());
        emit lastErrorChanged();
        emit languageListDownloaded(false);
        qWarning() << m_lastError;
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        m_lastError = "Invalid language list format";
        emit lastErrorChanged();
        emit languageListDownloaded(false);
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray languages = root["languages"].toArray();

    for (const QJsonValue& val : languages) {
        QJsonObject lang = val.toObject();
        QString code = lang["code"].toString();
        QString displayName = lang["name"].toString();
        QString nativeName = lang["nativeName"].toString();
        bool isRtl = lang["isRtl"].toBool(false);

        if (!code.isEmpty() && !m_languageMetadata.contains(code)) {
            m_languageMetadata[code] = QVariantMap{
                {"displayName", displayName},
                {"nativeName", nativeName.isEmpty() ? displayName : nativeName},
                {"isRtl", isRtl},
                {"isRemote", true}  // Mark as available for download
            };
        }
    }

    saveLanguageMetadata();
    m_availableLanguages = m_languageMetadata.keys();
    emit availableLanguagesChanged();
    emit languageListDownloaded(true);

    qDebug() << "Language list updated. Available:" << m_availableLanguages;
}

void TranslationManager::onLanguageFileFetched(QNetworkReply* reply)
{
    reply->deleteLater();
    m_downloading = false;
    QString langCode = m_downloadingLangCode;
    m_downloadingLangCode.clear();
    emit downloadingChanged();

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = QString("Failed to download %1: %2").arg(langCode, reply->errorString());
        emit lastErrorChanged();
        emit languageDownloaded(langCode, false, m_lastError);
        qWarning() << m_lastError;
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        m_lastError = "Invalid translation file format";
        emit lastErrorChanged();
        emit languageDownloaded(langCode, false, m_lastError);
        return;
    }

    // Save the downloaded file
    QFile file(languageFilePath(langCode));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
    }

    // Update metadata if provided
    QJsonObject root = doc.object();
    if (root.contains("displayName") || root.contains("nativeName")) {
        m_languageMetadata[langCode] = QVariantMap{
            {"displayName", root["displayName"].toString(langCode)},
            {"nativeName", root["nativeName"].toString(langCode)},
            {"isRtl", root["isRtl"].toBool(false)},
            {"isRemote", false}  // Now downloaded locally
        };
        saveLanguageMetadata();
    }

    // Reload if this is the current language
    if (langCode == m_currentLanguage) {
        loadTranslations();
        recalculateUntranslatedCount();
        m_translationVersion++;
        emit translationsChanged();
    }

    emit languageDownloaded(langCode, true, QString());
    qDebug() << "Downloaded language:" << langCode;
}

void TranslationManager::exportTranslation(const QString& filePath)
{
    // Allow exporting any language including English customizations
    QJsonObject root;
    root["language"] = m_currentLanguage;
    root["displayName"] = getLanguageDisplayName(m_currentLanguage);
    root["nativeName"] = getLanguageNativeName(m_currentLanguage);
    root["isRtl"] = isRtlLanguage(m_currentLanguage);

    QJsonObject translations;
    for (auto it = m_translations.constBegin(); it != m_translations.constEnd(); ++it) {
        translations[it.key()] = it.value();
    }
    root["translations"] = translations;

    // Also include untranslated strings with empty values for reference
    QJsonObject untranslated;
    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        if (!m_translations.contains(it.key())) {
            untranslated[it.key()] = it.value();  // English fallback as hint
        }
    }
    root["untranslated"] = untranslated;

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
        qDebug() << "Exported translation to:" << filePath;
    } else {
        m_lastError = QString("Failed to write file: %1").arg(filePath);
        emit lastErrorChanged();
    }
}

void TranslationManager::importTranslation(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = QString("Failed to open file: %1").arg(filePath);
        emit lastErrorChanged();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        m_lastError = "Invalid translation file format";
        emit lastErrorChanged();
        return;
    }

    QJsonObject root = doc.object();
    QString langCode = root["language"].toString();

    if (langCode.isEmpty()) {
        m_lastError = "Translation file missing language code";
        emit lastErrorChanged();
        return;
    }

    // Save the imported file
    QFile destFile(languageFilePath(langCode));
    if (destFile.open(QIODevice::WriteOnly)) {
        destFile.write(data);
        destFile.close();
    }

    // Update metadata
    m_languageMetadata[langCode] = QVariantMap{
        {"displayName", root["displayName"].toString(langCode)},
        {"nativeName", root["nativeName"].toString(langCode)},
        {"isRtl", root["isRtl"].toBool(false)}
    };
    saveLanguageMetadata();

    m_availableLanguages = m_languageMetadata.keys();
    emit availableLanguagesChanged();

    // If importing for current language, reload
    if (langCode == m_currentLanguage) {
        loadTranslations();
        recalculateUntranslatedCount();
        m_translationVersion++;
        emit translationsChanged();
    }

    qDebug() << "Imported translation for:" << langCode;
}

void TranslationManager::openGitHubSubmission()
{
    // Create a GitHub issue with the translation data pre-filled
    QString title = QString("Translation submission: %1").arg(getLanguageDisplayName(m_currentLanguage));

    QString body = QString("## Language: %1 (%2)\n\n"
                          "Translated %3 of %4 strings (%5%)\n\n"
                          "**Please attach your exported translation JSON file to this issue.**\n\n"
                          "You can export your translation from:\n"
                          "Settings -> Language -> Export Translation\n")
                       .arg(getLanguageDisplayName(m_currentLanguage))
                       .arg(m_currentLanguage)
                       .arg(m_translations.size())
                       .arg(m_stringRegistry.size())
                       .arg(m_stringRegistry.isEmpty() ? 0 : (m_translations.size() * 100 / m_stringRegistry.size()));

    QUrl url(GITHUB_ISSUES_URL);
    QUrlQuery query;
    query.addQueryItem("title", title);
    query.addQueryItem("body", body);
    query.addQueryItem("labels", "translation");
    url.setQuery(query);

    QDesktopServices::openUrl(url);
}

// --- Utility ---

QVariantList TranslationManager::getUntranslatedStrings() const
{
    QVariantList result;

    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        if (!m_translations.contains(it.key()) || m_translations.value(it.key()).isEmpty()) {
            result.append(QVariantMap{
                {"key", it.key()},
                {"fallback", it.value()}
            });
        }
    }

    return result;
}

QVariantList TranslationManager::getAllStrings() const
{
    QVariantList result;

    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        QString fallback = it.value();
        QString translation = m_translations.value(it.key());
        QString aiTranslation = m_aiTranslations.value(fallback);
        bool isTranslated = !translation.isEmpty();
        bool isAiGen = m_aiGenerated.contains(it.key());

        result.append(QVariantMap{
            {"key", it.key()},
            {"fallback", fallback},
            {"translation", translation},
            {"isTranslated", isTranslated},
            {"aiTranslation", aiTranslation},
            {"isAiGenerated", isAiGen}
        });
    }

    return result;
}

bool TranslationManager::isRtlLanguage(const QString& langCode) const
{
    if (m_languageMetadata.contains(langCode)) {
        return m_languageMetadata[langCode]["isRtl"].toBool();
    }

    // Common RTL languages
    static const QStringList rtlLanguages = {"ar", "he", "fa", "ur"};
    return rtlLanguages.contains(langCode);
}

QVariantList TranslationManager::getGroupedStrings() const
{
    // Group strings by their fallback (English) text
    QMap<QString, QStringList> fallbackToKeys;
    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        fallbackToKeys[it.value()].append(it.key());
    }

    QVariantList result;
    for (auto it = fallbackToKeys.constBegin(); it != fallbackToKeys.constEnd(); ++it) {
        QString fallback = it.key();
        QStringList keys = it.value();

        // Get AI translation for this fallback (shared across all keys)
        QString aiTranslation = m_aiTranslations.value(fallback);

        // Check translation status for all keys in this group
        QVariantList keysInfo;
        QString groupTranslation;
        bool hasAnyTranslation = false;
        bool allSameTranslation = true;
        bool allAiGenerated = true;
        bool anyAiGenerated = false;
        QString firstTranslation;

        for (const QString& key : keys) {
            QString translation = m_translations.value(key);
            bool isAiGen = m_aiGenerated.contains(key);

            if (!translation.isEmpty()) {
                if (!hasAnyTranslation) {
                    firstTranslation = translation;
                    groupTranslation = translation;
                    hasAnyTranslation = true;
                } else if (translation != firstTranslation) {
                    allSameTranslation = false;
                }
            }

            if (isAiGen) {
                anyAiGenerated = true;
            } else if (!translation.isEmpty()) {
                allAiGenerated = false;
            }

            keysInfo.append(QVariantMap{
                {"key", key},
                {"translation", translation},
                {"isTranslated", !translation.isEmpty()},
                {"isAiGenerated", isAiGen}
            });
        }

        // If there are mixed translations, consider it "split"
        bool isSplit = hasAnyTranslation && !allSameTranslation;

        // Group is AI-generated if all translated keys are AI-generated
        bool groupIsAiGenerated = hasAnyTranslation && allAiGenerated && anyAiGenerated;

        result.append(QVariantMap{
            {"fallback", fallback},
            {"translation", groupTranslation},
            {"aiTranslation", aiTranslation},
            {"keys", keysInfo},
            {"keyCount", keys.size()},
            {"isTranslated", hasAnyTranslation},
            {"isSplit", isSplit},
            {"isAiGenerated", groupIsAiGenerated}
        });
    }

    return result;
}

QStringList TranslationManager::getKeysForFallback(const QString& fallback) const
{
    QStringList keys;
    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        if (it.value() == fallback) {
            keys.append(it.key());
        }
    }
    return keys;
}

void TranslationManager::setGroupTranslation(const QString& fallback, const QString& translation)
{
    QStringList keys = getKeysForFallback(fallback);
    for (const QString& key : keys) {
        if (translation.isEmpty()) {
            m_translations.remove(key);
        } else {
            m_translations[key] = translation;
        }
        m_aiGenerated.remove(key);  // User edited, no longer AI-generated
    }

    saveTranslations();
    recalculateUntranslatedCount();
    m_translationVersion++;
    emit translationsChanged();
}

bool TranslationManager::isGroupSplit(const QString& fallback) const
{
    QStringList keys = getKeysForFallback(fallback);
    if (keys.size() <= 1) return false;

    QString firstTranslation;
    bool hasFirst = false;

    for (const QString& key : keys) {
        QString translation = m_translations.value(key);
        if (!translation.isEmpty()) {
            if (!hasFirst) {
                firstTranslation = translation;
                hasFirst = true;
            } else if (translation != firstTranslation) {
                return true;  // Different translations found
            }
        }
    }

    return false;
}

void TranslationManager::mergeGroupTranslation(const QString& key)
{
    // Find the fallback for this key
    QString fallback = m_stringRegistry.value(key);
    if (fallback.isEmpty()) return;

    // Find the most common translation among keys with this fallback
    QStringList keys = getKeysForFallback(fallback);
    QMap<QString, int> translationCounts;

    for (const QString& k : keys) {
        QString translation = m_translations.value(k);
        if (!translation.isEmpty()) {
            translationCounts[translation]++;
        }
    }

    // Find the most common translation
    QString mostCommon;
    int maxCount = 0;
    for (auto it = translationCounts.constBegin(); it != translationCounts.constEnd(); ++it) {
        if (it.value() > maxCount) {
            maxCount = it.value();
            mostCommon = it.key();
        }
    }

    // Set this key to use the most common translation
    if (!mostCommon.isEmpty()) {
        m_translations[key] = mostCommon;
        saveTranslations();
        m_translationVersion++;
        emit translationsChanged();
    }
}

int TranslationManager::uniqueStringCount() const
{
    QSet<QString> uniqueFallbacks;
    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        uniqueFallbacks.insert(it.value());
    }
    return uniqueFallbacks.size();
}

int TranslationManager::uniqueUntranslatedCount() const
{
    // Count unique fallback texts that have NO translation for ANY of their keys
    QMap<QString, bool> fallbackTranslated;

    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        QString fallback = it.value();
        QString translation = m_translations.value(it.key());

        if (!fallbackTranslated.contains(fallback)) {
            fallbackTranslated[fallback] = !translation.isEmpty();
        } else if (!translation.isEmpty()) {
            fallbackTranslated[fallback] = true;
        }
    }

    int untranslated = 0;
    for (auto it = fallbackTranslated.constBegin(); it != fallbackTranslated.constEnd(); ++it) {
        if (!it.value()) {
            untranslated++;
        }
    }

    return untranslated;
}

// --- Private helpers ---

void TranslationManager::loadTranslations()
{
    m_translations.clear();

    // Load translations for any language (including English customizations)
    QFile file(languageFilePath(m_currentLanguage));
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "No translation file for:" << m_currentLanguage;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Invalid translation file for:" << m_currentLanguage;
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject translations = root["translations"].toObject();

    for (auto it = translations.constBegin(); it != translations.constEnd(); ++it) {
        m_translations[it.key()] = it.value().toString();
    }

    qDebug() << "Loaded" << m_translations.size() << "translations for:" << m_currentLanguage;
}

void TranslationManager::saveTranslations()
{
    // Save translations for any language (including English customizations)
    QJsonObject root;
    root["language"] = m_currentLanguage;
    root["displayName"] = getLanguageDisplayName(m_currentLanguage);
    root["nativeName"] = getLanguageNativeName(m_currentLanguage);
    root["isRtl"] = isRtlLanguage(m_currentLanguage);

    QJsonObject translations;
    for (auto it = m_translations.constBegin(); it != m_translations.constEnd(); ++it) {
        translations[it.key()] = it.value();
    }
    root["translations"] = translations;

    QFile file(languageFilePath(m_currentLanguage));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void TranslationManager::loadLanguageMetadata()
{
    QString metaPath = translationsDir() + "/languages_meta.json";
    QFile file(metaPath);

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }

    QJsonObject root = doc.object();
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        m_languageMetadata[it.key()] = it.value().toObject().toVariantMap();
    }
}

void TranslationManager::saveLanguageMetadata()
{
    QJsonObject root;
    for (auto it = m_languageMetadata.constBegin(); it != m_languageMetadata.constEnd(); ++it) {
        root[it.key()] = QJsonObject::fromVariantMap(it.value());
    }

    QString metaPath = translationsDir() + "/languages_meta.json";
    QFile file(metaPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void TranslationManager::loadStringRegistry()
{
    QString regPath = translationsDir() + "/string_registry.json";
    QFile file(regPath);

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }

    QJsonObject root = doc.object();
    QJsonObject strings = root["strings"].toObject();

    for (auto it = strings.constBegin(); it != strings.constEnd(); ++it) {
        m_stringRegistry[it.key()] = it.value().toString();
    }
}

void TranslationManager::saveStringRegistry()
{
    QJsonObject root;
    root["version"] = "1.0";

    QJsonObject strings;
    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        strings[it.key()] = it.value();
    }
    root["strings"] = strings;

    QString regPath = translationsDir() + "/string_registry.json";
    QFile file(regPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

void TranslationManager::recalculateUntranslatedCount()
{
    // For English: count uncustomized strings
    // For other languages: count untranslated strings
    int count = 0;
    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        if (!m_translations.contains(it.key()) || m_translations.value(it.key()).isEmpty()) {
            count++;
        }
    }
    m_untranslatedCount = count;
    emit untranslatedCountChanged();
}

// --- AI Auto-Translation ---

bool TranslationManager::canAutoTranslate() const
{
    if (m_currentLanguage == "en") return false;
    if (m_autoTranslating) return false;

    QString provider = m_settings->aiProvider();
    if (provider == "openai" && !m_settings->openaiApiKey().isEmpty()) return true;
    if (provider == "anthropic" && !m_settings->anthropicApiKey().isEmpty()) return true;
    if (provider == "gemini" && !m_settings->geminiApiKey().isEmpty()) return true;
    if (provider == "ollama" && !m_settings->ollamaEndpoint().isEmpty() && !m_settings->ollamaModel().isEmpty()) return true;

    return false;
}

void TranslationManager::autoTranslate()
{
    if (!canAutoTranslate()) {
        m_lastError = "AI provider not configured. Set up an AI provider in Settings.";
        emit lastErrorChanged();
        emit autoTranslateFinished(false, m_lastError);
        return;
    }

    // Get unique untranslated fallback texts (more efficient - translate once, apply to all keys)
    QSet<QString> seenFallbacks;
    m_stringsToTranslate.clear();

    for (auto it = m_stringRegistry.constBegin(); it != m_stringRegistry.constEnd(); ++it) {
        QString fallback = it.value();
        // Skip if already translated (check if ANY key with this fallback is translated)
        bool hasTranslation = false;
        for (auto keyIt = m_stringRegistry.constBegin(); keyIt != m_stringRegistry.constEnd(); ++keyIt) {
            if (keyIt.value() == fallback && !m_translations.value(keyIt.key()).isEmpty()) {
                hasTranslation = true;
                break;
            }
        }

        if (!hasTranslation && !seenFallbacks.contains(fallback)) {
            seenFallbacks.insert(fallback);
            // Use fallback as both key and fallback for unique translation
            m_stringsToTranslate.append(QVariantMap{
                {"key", fallback},  // Use fallback as key for grouped translation
                {"fallback", fallback}
            });
        }
    }

    if (m_stringsToTranslate.isEmpty()) {
        emit autoTranslateFinished(true, "All strings are already translated!");
        return;
    }

    m_autoTranslating = true;
    m_autoTranslateCancelled = false;
    m_autoTranslateProgress = 0;
    m_autoTranslateTotal = m_stringsToTranslate.size();
    emit autoTranslatingChanged();
    emit autoTranslateProgressChanged();

    qDebug() << "Starting auto-translate of" << m_autoTranslateTotal << "unique strings to" << m_currentLanguage;

    sendNextAutoTranslateBatch();
}

void TranslationManager::cancelAutoTranslate()
{
    if (m_autoTranslating) {
        m_autoTranslateCancelled = true;
        m_autoTranslating = false;
        emit autoTranslatingChanged();
        emit autoTranslateFinished(false, "Translation cancelled");
    }
}

void TranslationManager::sendNextAutoTranslateBatch()
{
    if (m_autoTranslateCancelled || m_stringsToTranslate.isEmpty()) {
        m_autoTranslating = false;
        emit autoTranslatingChanged();
        if (!m_autoTranslateCancelled) {
            saveTranslations();
            saveAiTranslations();
            recalculateUntranslatedCount();
            m_translationVersion++;
            emit translationsChanged();
            emit autoTranslateFinished(true, QString("Translated %1 strings").arg(m_autoTranslateProgress));
        }
        return;
    }

    // Get next batch
    QVariantList batch;
    int batchSize = qMin(AUTO_TRANSLATE_BATCH_SIZE, m_stringsToTranslate.size());
    for (int i = 0; i < batchSize; i++) {
        batch.append(m_stringsToTranslate.takeFirst());
    }

    QString prompt = buildTranslationPrompt(batch);
    QString provider = m_settings->aiProvider();

    QNetworkRequest request;
    QByteArray postData;

    if (provider == "openai") {
        request.setUrl(QUrl("https://api.openai.com/v1/chat/completions"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", ("Bearer " + m_settings->openaiApiKey()).toUtf8());

        QJsonObject json;
        json["model"] = "gpt-4o-mini";  // Use mini for translation - cheaper and fast
        json["temperature"] = 0.3;
        QJsonArray messages;
        QJsonObject msg;
        msg["role"] = "user";
        msg["content"] = prompt;
        messages.append(msg);
        json["messages"] = messages;
        postData = QJsonDocument(json).toJson();

    } else if (provider == "anthropic") {
        request.setUrl(QUrl("https://api.anthropic.com/v1/messages"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("x-api-key", m_settings->anthropicApiKey().toUtf8());
        request.setRawHeader("anthropic-version", "2023-06-01");

        QJsonObject json;
        json["model"] = "claude-3-5-haiku-20241022";  // Use haiku for translation - cheaper and fast
        json["max_tokens"] = 4096;
        QJsonArray messages;
        QJsonObject msg;
        msg["role"] = "user";
        msg["content"] = prompt;
        messages.append(msg);
        json["messages"] = messages;
        postData = QJsonDocument(json).toJson();

    } else if (provider == "gemini") {
        QString apiKey = m_settings->geminiApiKey();
        request.setUrl(QUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + apiKey));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject json;
        QJsonArray contents;
        QJsonObject content;
        QJsonArray parts;
        QJsonObject part;
        part["text"] = prompt;
        parts.append(part);
        content["parts"] = parts;
        contents.append(content);
        json["contents"] = contents;
        postData = QJsonDocument(json).toJson();

    } else if (provider == "ollama") {
        QString endpoint = m_settings->ollamaEndpoint();
        if (!endpoint.endsWith("/")) endpoint += "/";
        request.setUrl(QUrl(endpoint + "api/generate"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject json;
        json["model"] = m_settings->ollamaModel();
        json["prompt"] = prompt;
        json["stream"] = false;
        postData = QJsonDocument(json).toJson();
    }

    QNetworkReply* reply = m_networkManager->post(request, postData);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onAutoTranslateBatchReply(reply);
    });
}

QString TranslationManager::buildTranslationPrompt(const QVariantList& strings) const
{
    QString langName = getLanguageDisplayName(m_currentLanguage);
    QString nativeName = getLanguageNativeName(m_currentLanguage);

    QString prompt = QString(
        "Translate the following English strings to %1 (%2).\n"
        "Return ONLY a JSON object with the translations, no explanation.\n"
        "The format must be exactly: {\"key\": \"translated text\", ...}\n"
        "Keep formatting like %1, %2, \\n exactly as-is.\n"
        "Be natural and idiomatic in %1.\n\n"
        "Strings to translate:\n"
    ).arg(langName, nativeName);

    for (const QVariant& v : strings) {
        QVariantMap item = v.toMap();
        QString key = item["key"].toString();
        QString fallback = item["fallback"].toString();
        prompt += QString("\"%1\": \"%2\"\n").arg(key, fallback.replace("\"", "\\\""));
    }

    return prompt;
}

void TranslationManager::onAutoTranslateBatchReply(QNetworkReply* reply)
{
    reply->deleteLater();

    if (m_autoTranslateCancelled) {
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        m_autoTranslating = false;
        m_lastError = "AI request failed: " + reply->errorString();
        emit autoTranslatingChanged();
        emit lastErrorChanged();
        emit autoTranslateFinished(false, m_lastError);
        return;
    }

    QByteArray data = reply->readAll();
    parseAutoTranslateResponse(data);

    // Continue with next batch
    sendNextAutoTranslateBatch();
}

void TranslationManager::parseAutoTranslateResponse(const QByteArray& data)
{
    QString provider = m_settings->aiProvider();
    QString content;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    if (provider == "openai") {
        content = root["choices"].toArray()[0].toObject()["message"].toObject()["content"].toString();
    } else if (provider == "anthropic") {
        content = root["content"].toArray()[0].toObject()["text"].toString();
    } else if (provider == "gemini") {
        content = root["candidates"].toArray()[0].toObject()["content"].toObject()["parts"].toArray()[0].toObject()["text"].toString();
    } else if (provider == "ollama") {
        content = root["response"].toString();
    }

    // Extract JSON from response (AI might include markdown code blocks)
    int jsonStart = content.indexOf('{');
    int jsonEnd = content.lastIndexOf('}');
    if (jsonStart >= 0 && jsonEnd > jsonStart) {
        content = content.mid(jsonStart, jsonEnd - jsonStart + 1);
    }

    // Parse translations and apply directly to empty keys
    // Note: The "key" in the response is actually the fallback text (since we translate unique texts)
    QJsonDocument transDoc = QJsonDocument::fromJson(content.toUtf8());
    if (transDoc.isObject()) {
        QJsonObject translations = transDoc.object();
        int count = 0;
        for (auto it = translations.constBegin(); it != translations.constEnd(); ++it) {
            QString fallbackText = it.key();
            QString translation = it.value().toString().trimmed();
            if (!translation.isEmpty()) {
                // Store in AI translations (for display in AI column)
                m_aiTranslations[fallbackText] = translation;

                // Apply to ALL keys with this fallback text that don't have a translation yet
                QStringList keys = getKeysForFallback(fallbackText);
                for (const QString& key : keys) {
                    if (m_translations.value(key).isEmpty()) {
                        m_translations[key] = translation;
                        m_aiGenerated.insert(key);  // Mark as AI-generated
                    }
                }

                // Update last translated text for UI feedback
                m_lastTranslatedText = fallbackText + " → " + translation;
                emit lastTranslatedTextChanged();

                count++;
            }
        }
        m_autoTranslateProgress += count;
        emit autoTranslateProgressChanged();

        qDebug() << "AI translated" << count << "strings, progress:" << m_autoTranslateProgress << "/" << m_autoTranslateTotal;
    } else {
        qWarning() << "Failed to parse AI translation response:" << content.left(200);
    }
}

// --- AI Translation Management ---

QString TranslationManager::getAiTranslation(const QString& fallback) const
{
    return m_aiTranslations.value(fallback);
}

bool TranslationManager::isAiGenerated(const QString& key) const
{
    return m_aiGenerated.contains(key);
}

void TranslationManager::copyAiToFinal(const QString& fallback)
{
    QString aiTranslation = m_aiTranslations.value(fallback);
    if (aiTranslation.isEmpty()) return;

    QStringList keys = getKeysForFallback(fallback);
    for (const QString& key : keys) {
        m_translations[key] = aiTranslation;
        m_aiGenerated.insert(key);  // Mark as AI-generated
    }

    saveTranslations();
    recalculateUntranslatedCount();
    m_translationVersion++;
    emit translationsChanged();
}

void TranslationManager::loadAiTranslations()
{
    m_aiTranslations.clear();
    m_aiGenerated.clear();

    if (m_currentLanguage == "en") {
        return;
    }

    QString aiPath = translationsDir() + "/" + m_currentLanguage + "_ai.json";
    QFile file(aiPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }

    QJsonObject root = doc.object();

    // Load AI translations (fallback -> translation)
    QJsonObject translations = root["translations"].toObject();
    for (auto it = translations.constBegin(); it != translations.constEnd(); ++it) {
        m_aiTranslations[it.key()] = it.value().toString();
    }

    // Load AI-generated flags (list of keys)
    QJsonArray generated = root["generated"].toArray();
    for (const QJsonValue& val : generated) {
        m_aiGenerated.insert(val.toString());
    }

    qDebug() << "Loaded" << m_aiTranslations.size() << "AI translations for:" << m_currentLanguage;
}

void TranslationManager::saveAiTranslations()
{
    if (m_currentLanguage == "en") {
        return;
    }

    QString aiPath = translationsDir() + "/" + m_currentLanguage + "_ai.json";

    if (m_aiTranslations.isEmpty()) {
        QFile::remove(aiPath);
        return;
    }

    QJsonObject root;
    root["language"] = m_currentLanguage;

    // Save AI translations
    QJsonObject translations;
    for (auto it = m_aiTranslations.constBegin(); it != m_aiTranslations.constEnd(); ++it) {
        translations[it.key()] = it.value();
    }
    root["translations"] = translations;

    // Save AI-generated flags
    QJsonArray generated;
    for (const QString& key : m_aiGenerated) {
        generated.append(key);
    }
    root["generated"] = generated;

    QFile file(aiPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}
