#pragma once

#include "vigine/format/documentimporters.h"

namespace vigine::format
{
class PdfImporter final : public IDocumentImporter
{
  public:
    [[nodiscard]] std::string formatName() const override { return "pdf"; }
    [[nodiscard]] std::optional<DiagramModel> importFile(const std::string &path,
                                                         std::size_t maxRunsPerPage) const override;
};
} // namespace vigine::format
