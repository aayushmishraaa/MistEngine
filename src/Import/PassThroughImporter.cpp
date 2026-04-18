#include "Import/PassThroughImporter.h"

#include <cctype>
#include <system_error>
#include <utility>

namespace Mist::Import {

// --- ImportSettings ---

void ImportSettings::Set(std::string_view key, std::string value) {
    m_Values[std::string(key)] = std::move(value);
}

std::string ImportSettings::GetOr(std::string_view key, std::string_view fallback) const {
    auto it = m_Values.find(std::string(key));
    if (it == m_Values.end()) return std::string(fallback);
    return it->second;
}

// --- ImporterRegistry ---

ImporterRegistry& ImporterRegistry::Instance() {
    static ImporterRegistry inst;
    return inst;
}

namespace {
std::string normExt(std::string_view ext) {
    std::string out(ext);
    for (auto& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}
} // namespace

void ImporterRegistry::Register(std::shared_ptr<IAssetImporter> importer) {
    if (!importer) return;
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto ext : importer->GetExtensions()) {
        auto key = normExt(ext);
        // First-registered wins per plan contract.
        m_ByExtension.try_emplace(std::move(key), importer);
    }
}

std::shared_ptr<IAssetImporter> ImporterRegistry::Get(std::string_view extension) const {
    std::string key = normExt(extension);
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_ByExtension.find(key);
    if (it == m_ByExtension.end()) return nullptr;
    return it->second;
}

void ImporterRegistry::Clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ByExtension.clear();
}

// --- PassThroughImporter ---

PassThroughImporter::PassThroughImporter(std::vector<std::string> extensions)
    : m_Extensions(std::move(extensions)) {}

std::vector<std::string_view> PassThroughImporter::GetExtensions() const {
    std::vector<std::string_view> out;
    out.reserve(m_Extensions.size());
    for (const auto& s : m_Extensions) out.emplace_back(s);
    return out;
}

std::filesystem::path PassThroughImporter::Import(const std::filesystem::path& source,
                                                   const std::filesystem::path& outputDir,
                                                   const ImportSettings&         /*settings*/) {
    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);
    if (ec) return {};

    auto dest = outputDir / source.filename();
    std::filesystem::copy_file(source, dest,
                               std::filesystem::copy_options::overwrite_existing,
                               ec);
    if (ec) return {};
    return dest;
}

} // namespace Mist::Import
