#include "OpenMedView/DicomViewer.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <itkImageIOFactory.h>
#include <vtkGDCMImageReader.h>
#include <vtkImageMapToWindowLevelColors.h>
#include <vtkImageData.h>
#include <vtkPNGWriter.h>
#include <vtkSmartPointer.h>

namespace openmedview {

namespace {

std::filesystem::path DefaultPreviewPath(const std::filesystem::path& inputPath)
{
    const std::filesystem::path parentPath = inputPath.parent_path();
    const std::filesystem::path stem = inputPath.stem();
    return parentPath / (stem.string() + "_preview.png");
}

} // namespace

bool GenerateDicomPreview(const std::filesystem::path& inputPath,
                          const std::filesystem::path& outputPath,
                          std::ostream& output)
{
    if (!std::filesystem::exists(inputPath) || !std::filesystem::is_regular_file(inputPath)) {
        output << "Preview failed: input file does not exist or is not a regular file: " << inputPath << '\n';
        return false;
    }

    const std::filesystem::path resolvedOutputPath = outputPath.empty() ? DefaultPreviewPath(inputPath) : outputPath;

    if (!resolvedOutputPath.has_parent_path()) {
        output << "Preview failed: output path must include a directory or filename.\n";
        return false;
    }

    const std::filesystem::path parentDirectory = resolvedOutputPath.parent_path();
    if (!parentDirectory.empty()) {
        std::filesystem::create_directories(parentDirectory);
    }

    // ITK is used here as a lightweight validation layer for the image IO stack.
    auto imageIo = itk::ImageIOFactory::CreateImageIO(inputPath.string().c_str(), itk::IOFileMode::ReadMode);
    if (imageIo) {
        output << "ITK IO backend: " << imageIo->GetNameOfClass() << '\n';
    }

    auto reader = vtkSmartPointer<vtkGDCMImageReader>::New();
    reader->SetFileName(inputPath.string().c_str());
    reader->Update();

    if (!reader->GetOutput()) {
        output << "Preview failed: could not read DICOM image data from " << inputPath << '\n';
        return false;
    }

    auto windowLevel = vtkSmartPointer<vtkImageMapToWindowLevelColors>::New();
    windowLevel->SetInputData(reader->GetOutput());
    windowLevel->SetWindow(400);
    windowLevel->SetLevel(40);

    auto writer = vtkSmartPointer<vtkPNGWriter>::New();
    writer->SetFileName(resolvedOutputPath.string().c_str());
    writer->SetInputConnection(windowLevel->GetOutputPort());
    writer->Write();

    output << "Generated DICOM preview: " << resolvedOutputPath << '\n';
    return true;
}

} // namespace openmedview
