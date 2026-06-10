#include "vigine/format/documentimporters.h"

// Without the vendored pdfio backend (no zlib on the host) the importer
// still links everywhere and simply reports "cannot parse"; consumers keep
// one code path on every platform.
#if !defined(VIGINEFORMAT_HAS_PDFIO)

#include <optional>
#include <string>

namespace vigine::format
{
std::optional<DiagramModel> PdfImporter::importFile(const std::string &path,
                                                    std::size_t maxRunsPerPage) const
{
    (void)path;
    (void)maxRunsPerPage;
    return std::nullopt;
}
} // namespace vigine::format

#else

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include <pdfio.h>

namespace vigine::format
{
namespace
{
std::string truncateLabel(std::string text)
{
    constexpr std::size_t kMaxLabel = 40;
    // Control bytes inside a run would garble the SDF label.
    for (char &ch : text)
        if (static_cast<unsigned char>(ch) < 0x20)
            ch = ' ';
    if (text.size() > kMaxLabel)
    {
        text.resize(kMaxLabel - 2);
        text += "..";
    }
    return text;
}

// The default pdfio error handler prints and aborts the open; recording the
// message and answering "stop" keeps malformed input a quiet nullopt.
bool recordError(pdfio_file_t *, const char *message, void *data)
{
    if (data != nullptr && message != nullptr)
        static_cast<std::string *>(data)->assign(message);
    return false;
}

// Pulls the first parenthesised string literals out of a page's decoded
// content stream -- in practice those are the page's text-show arguments.
// Handles \-escapes and nested parentheses the way the PDF grammar does.
std::vector<std::string> firstTextRuns(pdfio_obj_t *page, std::size_t maxRuns)
{
    std::vector<std::string> runs;
    const std::size_t        streamCount = pdfioPageGetNumStreams(page);
    for (std::size_t streamIndex = 0; streamIndex < streamCount && runs.size() < maxRuns;
         ++streamIndex)
    {
        pdfio_stream_t *stream = pdfioPageOpenStream(page, streamIndex, /*decode=*/true);
        if (stream == nullptr)
            continue;
        std::string content;
        char        buffer[8192];
        ssize_t     bytesRead = 0;
        constexpr std::size_t kMaxContent = 1u << 20;
        while ((bytesRead = pdfioStreamRead(stream, buffer, sizeof(buffer))) > 0 &&
               content.size() < kMaxContent)
            content.append(buffer, static_cast<std::size_t>(bytesRead));
        pdfioStreamClose(stream);

        std::string current;
        int         depth = 0;
        for (std::size_t index = 0; index < content.size() && runs.size() < maxRuns; ++index)
        {
            const char ch = content[index];
            if (depth == 0)
            {
                if (ch == '(')
                {
                    depth   = 1;
                    current = {};
                }
                continue;
            }
            if (ch == '\\' && index + 1 < content.size())
            {
                current.push_back(content[++index]);
                continue;
            }
            if (ch == '(')
            {
                ++depth;
                current.push_back(ch);
                continue;
            }
            if (ch == ')')
            {
                --depth;
                if (depth == 0)
                {
                    if (!current.empty())
                        runs.push_back(current);
                }
                else
                {
                    current.push_back(ch);
                }
                continue;
            }
            current.push_back(ch);
        }
    }
    return runs;
}
} // namespace

std::optional<DiagramModel> PdfImporter::importFile(const std::string &path,
                                                    std::size_t        maxRunsPerPage) const
{
    std::string   error;
    pdfio_file_t *pdf = pdfioFileOpen(path.c_str(), nullptr, nullptr, recordError, &error);
    if (pdf == nullptr)
        return std::nullopt;

    DiagramModel model;
    const char  *title = pdfioFileGetTitle(pdf);
    std::string  rootLabel = (title != nullptr && title[0] != '\0') ? title : path;
    const std::size_t slash = rootLabel.find_last_of('/');
    if (title == nullptr || title[0] == '\0')
        rootLabel = slash == std::string::npos ? rootLabel : rootLabel.substr(slash + 1);
    model.addNode("doc", truncateLabel(rootLabel));

    const std::size_t pageCount = pdfioFileGetNumPages(pdf);
    for (std::size_t pageIndex = 0; pageIndex < pageCount; ++pageIndex)
    {
        pdfio_obj_t *page = pdfioFileGetPage(pdf, pageIndex);
        if (page == nullptr)
            continue;
        const std::string pageId = "page" + std::to_string(pageIndex + 1);
        model.addNode(pageId, "Page " + std::to_string(pageIndex + 1));
        model.addEdge("doc", pageId);
        std::size_t runIndex = 0;
        for (const auto &run : firstTextRuns(page, maxRunsPerPage))
        {
            const std::string runId = pageId + "#t" + std::to_string(runIndex++);
            model.addNode(runId, truncateLabel(run));
            model.addEdge(pageId, runId);
        }
    }
    pdfioFileClose(pdf);

    if (model.nodes.size() < 2)
        return std::nullopt; // no pages resolved -- nothing to draw
    return model;
}

} // namespace vigine::format

#endif // VIGINEFORMAT_HAS_PDFIO
