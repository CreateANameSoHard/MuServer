#include "../include/StaticFile.h"
#include "../include/logger.h"

using FileType = StaticFile::FileType;
const std::unordered_map<StaticFile::FileType, std::string> StaticFile::fileTypeMap_ = {
    {FileType::ktext_html, "text/html"},
    {FileType::ktext_css, "text/css"},
    {FileType::kapplication_javascript, "application/javascript"},
    {FileType::kapplication_json, "application/json"},
    {FileType::ktext_plain, "text/plain"},
    {FileType::kapplication_xml, "application/xml"},
    {FileType::kimage_jpeg, "image/jpeg"},
    {FileType::kimage_png, "image/png"},
    {FileType::kimage_gif, "image/gif"},
    {FileType::kimage_webp, "image/webp"},
    {FileType::kimage_svg_xml, "image/svg+xml"},
    {FileType::kimage_x_icon, "image/x-icon"},
    {FileType::kvideo_mp4, "video/mp4"},
    {FileType::kvideo_webm, "video/webm"},
    {FileType::kaudio_mpeg, "audio/mpeg"},
    {FileType::kapplication_pdf, "application/pdf"},
    {FileType::kapplication_zip, "application/zip"},
    {FileType::kapplication_octet_stream, "application/octet-stream"}
};

StaticFile::StaticFile(std::string path, off_t offset,size_t len, FileType filetype)
    : path_(path),
    offset_(offset),
      fileSize_(len),
      type_(filetype)
{
}

StaticFile::~StaticFile()
{
    //文件控制权交给HttpServer
}

