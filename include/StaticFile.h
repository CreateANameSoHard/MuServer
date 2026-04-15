#pragma once
#include <string>
#include <unordered_map>

// use for http file send
class StaticFile
{
public:
    enum class FileType
    {
        ktext_html,
        ktext_css,
        kapplication_javascript,
        kapplication_json,
        ktext_plain,
        kapplication_xml,
        kimage_jpeg,
        kimage_png,
        kimage_gif,
        kimage_webp,
        kimage_svg_xml,
        kimage_x_icon,
        kvideo_mp4,
        kvideo_webm,
        kaudio_mpeg,
        kapplication_pdf,
        kapplication_zip,
        kapplication_octet_stream
    };
    StaticFile(std::string path, off_t offset, size_t len, FileType filetype = FileType::ktext_html);
    ~StaticFile();

    void setFileSize(size_t size) { fileSize_ = size; }
    const size_t getFileSize() const { return fileSize_; }

    void setType(FileType type) { type_ = type; }
    const FileType getType() const { return type_; }
    const std::string getTypeString() const
    {
        auto it = fileTypeMap_.find(getType());
        return it != fileTypeMap_.end() ? it->second : "application/octet-stream";
    }

    std::string path() const { return path_; }

    void setOffset(off_t offset) { offset_ = offset; }
    off_t getOffset() const { return offset_; }

    // const int fd() const { return fd_; }

private:
    std::string path_;
    off_t offset_;
    size_t fileSize_;
    FileType type_;
    // int fd_;

    const static std::unordered_map<FileType, std::string> fileTypeMap_;
};