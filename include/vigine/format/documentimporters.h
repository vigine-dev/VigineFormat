#pragma once

#include "vigine/format/diagrammodel.h"
#include "vigine/format/vigineformat_export.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace vigine::format
{
// A document importer draws a document's structure as a diagram. Document
// formats are binary and file-oriented, so unlike the text importers they take
// a PATH.
class VIGINEFORMAT_API IDocumentImporter
{
  public:
    virtual ~IDocumentImporter();

    [[nodiscard]] virtual std::string formatName() const = 0;

    [[nodiscard]] virtual std::optional<DiagramModel>
    importFile(const std::string &path, std::size_t maxRunsPerPage = 6) const = 0;

  protected:
    IDocumentImporter() = default;

  public:
    IDocumentImporter(const IDocumentImporter &)            = delete;
    IDocumentImporter &operator=(const IDocumentImporter &) = delete;
    IDocumentImporter(IDocumentImporter &&)                 = delete;
    IDocumentImporter &operator=(IDocumentImporter &&)      = delete;
};

// PDF: the title at the root, one node per page, the first text runs of each
// page as children. Built with the PDF backend when the library is configured
// with VIGINEFORMAT_WITH_PDF (the default); without it the importer reports
// "cannot parse".
[[nodiscard]] VIGINEFORMAT_API std::unique_ptr<IDocumentImporter> createPdfImporter();

} // namespace vigine::format
