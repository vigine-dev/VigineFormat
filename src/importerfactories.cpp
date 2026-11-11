#include "vigine/format/diagramimporters.h"
#include "vigine/format/documentimporters.h"
#include "vigine/format/pointcloud.h"

#include "diagramimporterclasses.h"
#include "pdfimporterclass.h"
#include "pointimporterclasses.h"

namespace vigine::format
{
std::unique_ptr<IDiagramImporter> createDotImporter()
{
    return std::make_unique<DotImporter>();
}

std::unique_ptr<IDiagramImporter> createMermaidImporter()
{
    return std::make_unique<MermaidImporter>();
}

std::unique_ptr<IDiagramImporter> createGraphMlImporter()
{
    return std::make_unique<GraphMlImporter>();
}

std::unique_ptr<IDiagramImporter> createEdgeListImporter()
{
    return std::make_unique<EdgeListImporter>();
}

std::unique_ptr<IDiagramImporter> createJsonTreeImporter()
{
    return std::make_unique<JsonTreeImporter>();
}

std::unique_ptr<IDiagramImporter> createPlantUmlImporter()
{
    return std::make_unique<PlantUmlImporter>();
}

std::unique_ptr<IDiagramImporter> createFlameGraphImporter()
{
    return std::make_unique<FlameGraphImporter>();
}

std::unique_ptr<IDiagramImporter> createGraphQlImporter()
{
    return std::make_unique<GraphQlImporter>();
}

std::unique_ptr<IDiagramImporter> createProtobufImporter()
{
    return std::make_unique<ProtobufImporter>();
}

std::unique_ptr<IDiagramImporter> createSqlErdImporter()
{
    return std::make_unique<SqlErdImporter>();
}

std::unique_ptr<IDiagramImporter> createYamlTreeImporter()
{
    return std::make_unique<YamlTreeImporter>();
}

std::unique_ptr<IDiagramImporter> createTomlTreeImporter()
{
    return std::make_unique<TomlTreeImporter>();
}

std::unique_ptr<IDiagramImporter> createOpenApiImporter()
{
    return std::make_unique<OpenApiImporter>();
}

std::unique_ptr<IDocumentImporter> createPdfImporter()
{
    return std::make_unique<PdfImporter>();
}

std::unique_ptr<IPointCloudImporter> createObjImporter()
{
    return std::make_unique<ObjImporter>();
}

std::unique_ptr<IPointCloudImporter> createStlImporter()
{
    return std::make_unique<StlImporter>();
}

std::unique_ptr<IPointCloudImporter> createPlyImporter()
{
    return std::make_unique<PlyImporter>();
}

std::unique_ptr<IPointCloudImporter> createXyzImporter()
{
    return std::make_unique<XyzImporter>();
}

std::unique_ptr<IPointCloudImporter> createVoxImporter()
{
    return std::make_unique<VoxImporter>();
}

} // namespace vigine::format
