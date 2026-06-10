#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "vigine/format/diagrammodel.h"

namespace vigine::format
{
// PDF importer: draws a document's structure as a tree -- the title at the
// root, one node per page, and the first text runs of each page as children.
// PDF is a binary, file-oriented format (the backend reads by filename), so
// unlike the text importers this one takes a PATH. Built when the library is
// configured with VIGINEFORMAT_WITH_PDF (the default).
class PdfImporter final
{
  public:
    [[nodiscard]] std::string formatName() const { return "pdf"; }
    [[nodiscard]] std::optional<DiagramModel> importFile(const std::string &path,
                                                         std::size_t maxRunsPerPage = 6) const;
};

} // namespace vigine::format
