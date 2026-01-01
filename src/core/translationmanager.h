#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMap>
#include <QVariantMap>
#include <QStringList>

class Settings;

class TranslationManager : public QObject {
    Q_OBJECT

    // Current language settings
    Q_PROPERTY(QString currentLanguage READ currentLanguage WRITE setCurrentLanguage NOTIFY currentLanguageChanged)
    Q_PROPERTY(bool editModeEnabled READ editModeEnabled WRITE setEditModeEnabled NOTIFY editModeEnabledChanged)

    // Translation status
    Q_PROPERTY(int untranslatedCount READ untranslatedCount NOTIFY untranslatedCountChanged)
    Q_PROPERTY(int totalStringCount READ totalStringCount NOTIFY totalStringCountChanged)
    Q_PROPERTY(QStringList availableLanguages READ availableLanguages NOTIFY availableLanguagesChanged)

    // Network status
    Q_PROPERTY(bool downloading READ isDownloading NOTIFY downloadingChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

    // AI translation status
    Q_PROPERTY(bool autoTranslating READ isAutoTranslating NOTIFY autoTranslatingChanged)
    Q_PROPERTY(int autoTranslateProgress READ autoTranslateProgress NOTIFY autoTranslateProgressChanged)
    Q_PROPERTY(int autoTranslateTotal READ autoTranslateTotal NOTIFY autoTranslateProgressChanged)
    Q_PROPERTY(QString lastTranslatedText READ lastTranslatedText NOTIFY lastTranslatedTextChanged)

    // Version counter - increments when translations change, used for QML reactivity
    Q_PROPERTY(int translationVersion READ translationVersion NOTIFY translationsChanged)

public:
    explicit TranslationManager(Settings* settings, QObject* parent = nullptr);

    // Properties
    QString currentLanguage() const;
    void setCurrentLanguage(const QString& lang);
    bool editModeEnabled() const;
    void setEditModeEnabled(bool enabled);
    int untranslatedCount() const;
    int totalStringCount() const;
    QStringList availableLanguages() const;
    bool isDownloading() const;
    QString lastError() const;
    int translationVersion() const { return m_translationVersion; }
    bool isAutoTranslating() const { return m_autoTranslating; }
    int autoTranslateProgress() const { return m_autoTranslateProgress; }
    int autoTranslateTotal() const { return m_autoTranslateTotal; }
    QString lastTranslatedText() const { return m_lastTranslatedText; }

    // Translation lookup (auto-registers strings)
    Q_INVOKABLE QString translate(const QString& key, const QString& fallback);
    Q_INVOKABLE bool hasTranslation(const QString& key) const;

    // Translation editing
    Q_INVOKABLE void setTranslation(const QString& key, const QString& translation);
    Q_INVOKABLE void deleteTranslation(const QString& key);

    // Language management
    Q_INVOKABLE void addLanguage(const QString& langCode, const QString& displayName, const QString& nativeName = QString());
    Q_INVOKABLE void deleteLanguage(const QString& langCode);
    Q_INVOKABLE QString getLanguageDisplayName(const QString& langCode) const;
    Q_INVOKABLE QString getLanguageNativeName(const QString& langCode) const;

    // String registry - tracks all known keys in the app
    Q_INVOKABLE void registerString(const QString& key, const QString& fallback);

    // Community translations
    Q_INVOKABLE void downloadLanguageList();
    Q_INVOKABLE void downloadLanguage(const QString& langCode);
    Q_INVOKABLE void exportTranslation(const QString& filePath);
    Q_INVOKABLE void importTranslation(const QString& filePath);
    Q_INVOKABLE void openGitHubSubmission();

    // Utility
    Q_INVOKABLE QVariantList getUntranslatedStrings() const;
    Q_INVOKABLE QVariantList getAllStrings() const;
    Q_INVOKABLE QVariantList getGroupedStrings() const;  // Groups by fallback text
    Q_INVOKABLE QStringList getKeysForFallback(const QString& fallback) const;
    Q_INVOKABLE void setGroupTranslation(const QString& fallback, const QString& translation);  // Sets for all keys with fallback
    Q_INVOKABLE bool isGroupSplit(const QString& fallback) const;  // True if keys have different translations
    Q_INVOKABLE void mergeGroupTranslation(const QString& key);  // Resets key to use group's common translation
    Q_INVOKABLE bool isRtlLanguage(const QString& langCode) const;
    Q_INVOKABLE int uniqueStringCount() const;  // Count of unique fallback texts
    Q_INVOKABLE int uniqueUntranslatedCount() const;  // Count of unique untranslated fallback texts

    // AI auto-translation
    Q_INVOKABLE void autoTranslate();
    Q_INVOKABLE void cancelAutoTranslate();
    Q_INVOKABLE bool canAutoTranslate() const;

    // AI translation tracking
    Q_INVOKABLE QString getAiTranslation(const QString& fallback) const;  // Get AI translation for fallback text
    Q_INVOKABLE bool isAiGenerated(const QString& key) const;  // Check if translation is unmodified AI output
    Q_INVOKABLE void copyAiToFinal(const QString& fallback);  // Copy AI translation to final for all keys

signals:
    void currentLanguageChanged();
    void editModeEnabledChanged();
    void untranslatedCountChanged();
    void totalStringCountChanged();
    void availableLanguagesChanged();
    void downloadingChanged();
    void lastErrorChanged();

    void translationsChanged();
    void translationChanged(const QString& key);
    void languageDownloaded(const QString& langCode, bool success, const QString& error);
    void languageListDownloaded(bool success);

    void autoTranslatingChanged();
    void autoTranslateProgressChanged();
    void autoTranslateFinished(bool success, const QString& message);
    void lastTranslatedTextChanged();

private slots:
    void onLanguageListFetched(QNetworkReply* reply);
    void onLanguageFileFetched(QNetworkReply* reply);
    void onAutoTranslateBatchReply(QNetworkReply* reply);

private:
    void loadTranslations();
    void saveTranslations();
    void loadLanguageMetadata();
    void saveLanguageMetadata();
    void loadStringRegistry();
    void saveStringRegistry();
    void recalculateUntranslatedCount();
    QString translationsDir() const;
    QString languageFilePath(const QString& langCode) const;

    // AI translation helpers
    void sendNextAutoTranslateBatch();
    void parseAutoTranslateResponse(const QByteArray& data);
    QString buildTranslationPrompt(const QVariantList& strings) const;
    void loadAiTranslations();
    void saveAiTranslations();

    Settings* m_settings;
    QNetworkAccessManager* m_networkManager;

    QString m_currentLanguage;
    bool m_editModeEnabled = false;
    bool m_downloading = false;
    QString m_lastError;

    // translations[key] = translated_text
    QMap<QString, QString> m_translations;

    // Registry of all known string keys and their English fallbacks
    // registry[key] = english_fallback
    QMap<QString, QString> m_stringRegistry;

    // Language metadata: {langCode: {displayName, nativeName, isRtl}}
    QMap<QString, QVariantMap> m_languageMetadata;

    // List of available language codes (local + community)
    QStringList m_availableLanguages;

    int m_untranslatedCount = 0;
    int m_translationVersion = 0;

    // Track which language is being downloaded
    QString m_downloadingLangCode;

    // Dirty flag for batch saving string registry
    bool m_registryDirty = false;

    // AI auto-translation state
    bool m_autoTranslating = false;
    bool m_autoTranslateCancelled = false;
    int m_autoTranslateProgress = 0;
    int m_autoTranslateTotal = 0;
    QVariantList m_stringsToTranslate;
    QString m_lastTranslatedText;
    static constexpr int AUTO_TRANSLATE_BATCH_SIZE = 25;

    // AI translations - stored per unique fallback text (not per key)
    // m_aiTranslations[fallback] = AI-generated translation
    QMap<QString, QString> m_aiTranslations;

    // Set of keys whose current translation is unmodified AI output
    QSet<QString> m_aiGenerated;

    static constexpr const char* GITHUB_RAW_BASE = "https://raw.githubusercontent.com/Kulitorum/de1-qt-translations/main";
    static constexpr const char* GITHUB_ISSUES_URL = "https://github.com/Kulitorum/de1-qt-translations/issues/new";
};
