#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "vigine/format/documentimporters.h"

namespace
{
using namespace vigine::format;

// Assembles a minimal one-page uncompressed PDF with a correct xref table --
// offsets are computed, not hand-counted, so the file stays valid.
std::string buildTinyPdf(const std::string &text)
{
    const std::string content =
        "BT /F1 12 Tf 72 712 Td (" + text + ") Tj ET\n";
    std::vector<std::string> objects = {
        "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n",
        "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n",
        "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
        "/Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >>\nendobj\n",
        "4 0 obj\n<< /Length " + std::to_string(content.size()) + " >>\nstream\n" + content +
            "endstream\nendobj\n",
        "5 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>\nendobj\n",
    };

    std::string pdf = "%PDF-1.4\n";
    std::vector<std::size_t> offsets;
    for (const auto &object : objects)
    {
        offsets.push_back(pdf.size());
        pdf += object;
    }
    const std::size_t xrefStart = pdf.size();
    pdf += "xref\n0 " + std::to_string(objects.size() + 1) + "\n";
    pdf += "0000000000 65535 f \n";
    for (const std::size_t offset : offsets)
    {
        char line[32];
        std::snprintf(line, sizeof(line), "%010zu 00000 n \n", offset);
        pdf += line;
    }
    pdf += "trailer\n<< /Size " + std::to_string(objects.size() + 1) +
           " /Root 1 0 R >>\nstartxref\n" + std::to_string(xrefStart) + "\n%%EOF\n";
    return pdf;
}

TEST(PdfImporter, TinyDocumentBecomesPagesAndTextRuns)
{
    const std::string path = std::string(::testing::TempDir()) + "vigineformat_tiny.pdf";
    {
        std::ofstream out(path, std::ios::binary);
        out << buildTinyPdf("Hello CodeMap");
    }

    PdfImporter pdf;
    const auto  model = pdf.importFile(path);
    ASSERT_TRUE(model.has_value());
    const auto hasLabel = [&](const std::string &label) {
        return std::any_of(model->nodes.begin(), model->nodes.end(),
                           [&](const DiagramNode &node) { return node.label == label; });
    };
    EXPECT_TRUE(hasLabel("Page 1"));
    EXPECT_TRUE(hasLabel("Hello CodeMap"));
    std::remove(path.c_str());
}

TEST(PdfImporter, MissingOrBrokenFileYieldsNoModel)
{
    PdfImporter pdf;
    EXPECT_FALSE(pdf.importFile("/nonexistent/definitely_missing.pdf").has_value());
    const std::string path = std::string(::testing::TempDir()) + "vigineformat_broken.pdf";
    {
        std::ofstream out(path, std::ios::binary);
        out << "%PDF-1.4\nthis is not really a pdf";
    }
    EXPECT_FALSE(pdf.importFile(path).has_value());
    std::remove(path.c_str());
}
} // namespace
